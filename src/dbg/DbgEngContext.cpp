// Raw-DbgEng replacement for engextcpp's g_Ext singleton on the data path
// (engextcpp-replacement.md §7.1/§7.6). No precompiled header, no engextcpp.

#include "DbgEngContext.h"
#include "RemoteValue.h" // RemoteReadError

#include <windows.h>
#include <DbgEng.h>

#include <cctype>
#include <string>

namespace PyExt::Dbg {

	namespace {
		// Throw a RemoteReadError on a failed HRESULT with context.
		void check(HRESULT hr, const char* what)
		{
			if (FAILED(hr))
				throw RemoteReadError(std::string(what) + " failed (hr=0x"
					+ [hr] { char b[16]; sprintf_s(b, "%08lx", static_cast<unsigned long>(hr)); return std::string(b); }() + ")");
		}
	}


	struct DbgEngContext::Impl {
		IDebugControl* control = nullptr;
		IDebugSymbols3* symbols = nullptr;
		IDebugDataSpaces4* data = nullptr;
		IDebugSystemObjects* system = nullptr;
		std::shared_ptr<OutputSink> sink;

		~Impl()
		{
			if (system) system->Release();
			if (data) data->Release();
			if (symbols) symbols->Release();
			if (control) control->Release();
		}
	};


	DbgEngContext::DbgEngContext(IDebugClient* client, std::shared_ptr<OutputSink> sink)
		: impl_(std::make_unique<Impl>())
	{
		if (client == nullptr)
			throw RemoteReadError("DbgEngContext: null IDebugClient");

		check(client->QueryInterface(__uuidof(IDebugControl), reinterpret_cast<void**>(&impl_->control)), "QI(IDebugControl)");
		check(client->QueryInterface(__uuidof(IDebugSymbols3), reinterpret_cast<void**>(&impl_->symbols)), "QI(IDebugSymbols3)");
		check(client->QueryInterface(__uuidof(IDebugDataSpaces4), reinterpret_cast<void**>(&impl_->data)), "QI(IDebugDataSpaces4)");
		check(client->QueryInterface(__uuidof(IDebugSystemObjects), reinterpret_cast<void**>(&impl_->system)), "QI(IDebugSystemObjects)");

		impl_->sink = sink ? std::move(sink) : std::make_shared<ControlOutputSink>(impl_->control);
	}

	DbgEngContext::~DbgEngContext() = default;

	auto DbgEngContext::control() const -> IDebugControl* { return impl_->control; }
	auto DbgEngContext::symbols() const -> IDebugSymbols3* { return impl_->symbols; }
	auto DbgEngContext::data() const -> IDebugDataSpaces4* { return impl_->data; }
	auto DbgEngContext::system() const -> IDebugSystemObjects* { return impl_->system; }
	auto DbgEngContext::output() const -> OutputSink& { return *impl_->sink; }


	auto DbgEngContext::evaluateU64(std::string_view expression) const -> std::optional<std::uint64_t>
	{
		DEBUG_VALUE value{};
		std::string expr(expression);
		HRESULT hr = impl_->control->Evaluate(expr.c_str(), DEBUG_VALUE_INT64, &value, nullptr);
		if (FAILED(hr))
			return std::nullopt;
		return value.I64;
	}


	auto DbgEngContext::pointerSize() const -> int
	{
		// S_OK => 64-bit pointers, S_FALSE => 32-bit.
		return (impl_->control->IsPointer64Bit() == S_OK) ? 8 : 4;
	}


	auto DbgEngContext::qualifyPythonSymbol(std::string_view symbol) const -> std::optional<std::string>
	{
		// Find the loaded "python*" module (python, python3X, python3Xt) and
		// prefix the symbol with "<module>!". Replaces FindFirstModule("python???").
		ULONG loaded = 0, unloaded = 0;
		if (FAILED(impl_->symbols->GetNumberModules(&loaded, &unloaded)))
			return std::nullopt;

		for (ULONG i = 0; i < loaded; ++i) {
			ULONG64 base = 0;
			if (FAILED(impl_->symbols->GetModuleByIndex(i, &base)))
				continue;

			char name[256] = {};
			ULONG nameSize = 0;
			// DEBUG_MODNAME_MODULE = the short module name used in "mod!sym".
			if (FAILED(impl_->symbols->GetModuleNameString(DEBUG_MODNAME_MODULE, i, base,
					name, sizeof(name), &nameSize)))
				continue;

			// Match the versioned "pythonNNN" dll that actually carries the symbols
			// (python311, python314, python313t, python27, ...). Skip the bare
			// "python" launcher exe (no PyType_Type etc.) and the "python3.dll"
			// forwarder. This mirrors engextcpp's FindFirstModule("python???"),
			// which required digits after the prefix.
			std::string mod(name);
			if (mod.rfind("python", 0) == 0 && mod != "python3"
				&& mod.size() > 6 && std::isdigit(static_cast<unsigned char>(mod[6]))) {
				std::string result = mod;
				result += '!';
				result += symbol;
				return result;
			}
		}
		return std::nullopt;
	}

}
