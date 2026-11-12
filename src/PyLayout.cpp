#include "PyLayout.h"

#include "ExtHelpers.h"

#include <engextcpp.hpp>

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
using namespace std;

namespace PyExt::Layout {

	auto pointerSize() -> int
	{
		// engextcpp refreshes this for the current target each time it's handed a client,
		// which makes it far cheaper than evaluating `sizeof(void*)` in the debuggee.
		return static_cast<int>(g_Ext->m_PtrSize);
	}


	namespace {

		struct PythonModule
		{
			string name;
			ULONG64 base = 0;
		};


		auto findPythonModule() -> PythonModule
		{
			PythonModule module;
			auto& symbols = g_Ext->m_Symbols;

			// Locate the module through a symbol it defines rather than by guessing at the module's
			// name, which varies ("python311", "python315t", "python315_d", ...).
			// `_PyRuntime` (Python 3.7+) is used because it isn't part of the stable ABI. Stable ABI
			// symbols are also exported from python3.dll as forwarders, and could resolve to that instead.
			ULONG64 symbolOffset = 0;
			ULONG moduleIndex = 0;
			char nameBuffer[MAX_PATH] = {};
			if (SUCCEEDED(symbols->GetOffsetByName("_PyRuntime", &symbolOffset))
				&& SUCCEEDED(symbols->GetModuleByOffset(symbolOffset, 0, &moduleIndex, &module.base))
				&& SUCCEEDED(symbols->GetModuleNames(moduleIndex, module.base,
					nullptr, 0, nullptr, nameBuffer, sizeof(nameBuffer), nullptr, nullptr, 0, nullptr)))
			{
				module.name = nameBuffer;
				return module;
			}

			// Older versions are always named "python" followed by their two or three digit version.
			for (auto pattern : { "python???", "python??" }) {
				try {
					ExtBuffer<char> buffer;
					g_Ext->FindFirstModule(pattern, &buffer, 0); //< Throws when nothing matches.
					if (SUCCEEDED(symbols->GetModuleByModuleName(buffer.GetBuffer(), 0, nullptr, &module.base))) {
						module.name = buffer.GetBuffer();
						return module;
					}
				} catch (ExtException&) { }
			}

			return { };
		}


		struct TypeInfo
		{
			ULONG64 moduleBase = 0;
			ULONG typeId = 0;
		};


		// Looking anything up by name makes the debugger search symbols, which is slow enough to dominate
		// the time it takes to print a large container when it's repeated for every element.
		// These hold for as long as the target can't change underneath us. See resetCache().
		optional<PythonModule> cachedModule;
		map<string, TypeInfo, less<>> cachedTypes;

	}


	auto resetCache() -> void
	{
		cachedModule.reset();
		cachedTypes.clear();
	}


	auto pythonModuleName() -> string
	{
		if (!cachedModule.has_value())
			cachedModule = findPythonModule();
		return cachedModule->name;
	}


	auto qualifiedSymbolName(string_view symbolName) -> string
	{
		auto result = pythonModuleName();
		if (result.empty())
			return string(symbolName); //< Leave it to the debugger to search for.

		result += '!';
		result += symbolName;
		return result;
	}


	auto typedPointer(string_view typeName, Offset address) -> ExtRemoteTyped
	{
		auto cached = cachedTypes.find(typeName);
		if (cached == cachedTypes.end()) {
			TypeInfo info;
			bool found = false;
			{
				// Hide the wall of text GetSymbolTypeId prints when it can't find a type.
				ExtCaptureOutputA ignoreOut;
				ignoreOut.Start();
				found = SUCCEEDED(g_Ext->m_Symbols->GetSymbolTypeId(qualifiedSymbolName(typeName).c_str(), &info.typeId, &info.moduleBase));
			}

			// Let ExtRemoteTyped do its own search, and report the failure the way callers expect if that fails too.
			if (!found)
				return ExtRemoteTyped(string(typeName).c_str(), address, true);

			cached = cachedTypes.emplace(string(typeName), info).first;
		}

		ExtRemoteTyped typed;
		typed.Set(true, cached->second.moduleBase, cached->second.typeId, address);
		return typed;
	}


	auto versionHex() -> optional<uint32_t>
	{
		optional<uint32_t> version;
		utils::ignoreExtensionError([&] {
			ExtRemoteTyped remoteVersion("Py_Version");
			version = utils::readIntegral<uint32_t>(remoteVersion);
		});
		return version;
	}


	auto minorVersion() -> optional<int>
	{
		auto version = versionHex();
		if (!version.has_value())
			return nullopt;
		return static_cast<int>((*version >> 16) & 0xFF);
	}


	auto isFreeThreaded() -> bool
	{
		// Free-threaded builds record the owning thread of every object.
		bool hasThreadId = false;
		utils::ignoreExtensionError([&] {
			hasThreadId = ExtRemoteTyped("PyObject", 0, true).HasField("ob_tid");
		});
		return hasThreadId;
	}


	auto enumeratorName(string_view enumName, int64_t value) -> optional<string>
	{
		// Hide the wall of text GetSymbolTypeId prints when the enum isn't in the PDB.
		ExtCaptureOutputA ignoreOut;
		ignoreOut.Start();

		ULONG typeId = 0;
		ULONG64 moduleBase = 0;
		auto& symbols = g_Ext->m_Symbols3;
		if (FAILED(symbols->GetSymbolTypeId(qualifiedSymbolName(enumName).c_str(), &typeId, &moduleBase)))
			return nullopt;

		// Negative enumerators may be stored sign-extended or as their 32-bit pattern.
		char name[256] = {};
		if (FAILED(symbols->GetConstantName(moduleBase, typeId, static_cast<ULONG64>(value), name, sizeof(name), nullptr))
			&& FAILED(symbols->GetConstantName(moduleBase, typeId, static_cast<ULONG>(value), name, sizeof(name), nullptr)))
			return nullopt;

		if (name[0] == '\0')
			return nullopt;
		return name;
	}


	auto stackRefToAddress(Offset bits) -> Offset
	{
		// Python 3.13 (free-threaded) and 3.14+ keep ownership tags in the low bits, and can store a
		// small integer in place of a pointer. Objects are always at least 4-byte aligned, so masking
		// is harmless on the untagged pointers that older versions store.
		constexpr Offset tagMask = 3; //< Py_TAG_BITS
		constexpr Offset intTag = 3;  //< Py_INT_TAG
		if ((bits & tagMask) == intTag)
			return 0;
		return bits & ~tagMask;
	}


	auto managedDictSlotOffset() -> int64_t
	{
		// The pre-header holds the dict pointer, then the weakref list pointer further out.
		// Between it and the object are the two pointers of the GC header, except in free-threaded
		// builds which keep their GC state inside the object. See MANAGED_DICT_OFFSET.
		const auto slotsBeforeObject = isFreeThreaded() ? 1 : 3;
		return -static_cast<int64_t>(slotsBeforeObject) * pointerSize();
	}

}
