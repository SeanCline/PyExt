#include "PyTypeObject.h"

#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"
#include "../PyLayout.h"
#include "PyInterpreterState.h"
#include "PyThreadState.h"

#include <engextcpp.hpp>
#include <initializer_list>
#include <optional>
#include <string>
using namespace std;

namespace {

	using PyExt::Remote::RemoteType;

	// Static types are shared by every interpreter in the process, so their mutable state (such as
	// tp_dict) is kept per interpreter in `interp->types`. Returns that struct for the interpreter
	// the debugger's current thread belongs to, or for the main interpreter if it isn't a Python thread.
	auto currentInterpreterTypes() -> ExtRemoteTyped
	{
		RemoteType::Offset interpreterAddress = 0;
		utils::ignoreExtensionError([&] {
			ULONG systemThreadId = 0;
			if (FAILED(g_Ext->m_System->GetCurrentThreadSystemId(&systemThreadId)))
				return;
			auto threadState = PyExt::Remote::PyInterpreterState::findThreadStateBySystemThreadId(systemThreadId);
			if (threadState.has_value())
				interpreterAddress = threadState->interpreterAddress();
		});

		// The struct is named in full because some versions' symbols can't resolve the `PyInterpreterState` typedef.
		optional<ExtRemoteTyped> types;
		if (interpreterAddress != 0) {
			utils::ignoreExtensionError([&] {
				auto structName = PyExt::Layout::qualifiedSymbolName("_is");
				types = ExtRemoteTyped(structName.c_str(), interpreterAddress, true).Field("types");
			});
		}

		if (types.has_value())
			return *types;
		return ExtRemoteTyped("_PyRuntime.interpreters.main->types");
	}


	// Finds the tp_dict of a static type in whichever of the interpreter's tables holds its state.
	auto findStaticTypeDict(ExtRemoteTyped types, RemoteType::SSize index, RemoteType::Offset typeAddress) -> RemoteType::Offset
	{
		// Python 3.13 added a second table for the static types that extension modules define.
		// The two are indexed independently, so the same index is valid in both and the entry's
		// own `type` pointer is what says which table a type belongs to.
		for (auto tableName : { "builtins", "for_extensions" }) {
			if (!types.HasField(tableName))
				continue;

			auto table = types.Field(tableName);
			if (table.HasField("initialized"))  // Python >= 3.13
				table = table.Field("initialized");

			auto state = table.ArrayElement(index);
			if (state.HasField("type") && state.Field("type").GetPtr() != typeAddress)
				continue;
			return state.Field("tp_dict").GetPtr();
		}
		return 0;
	}

}

namespace PyExt::Remote {

	PyTypeObject::PyTypeObject(Offset objectAddress)
		: PyVarObject(objectAddress, "PyTypeObject")
	{
	}


	auto PyTypeObject::name() const -> string
	{
		ExtBuffer<char> buff;
		remoteType().Field("tp_name").Dereference().GetString(&buff);
		return buff.GetBuffer();
	}


	auto PyTypeObject::basicSize() const -> SSize
	{
		auto basicSize = remoteType().Field("tp_basicsize");
		return utils::readIntegral<SSize>(basicSize);
	}


	auto PyTypeObject::itemSize() const -> SSize
	{
		auto itemSize = remoteType().Field("tp_itemsize");
		return utils::readIntegral<SSize>(itemSize);
	}


	auto PyTypeObject::documentation() const -> string
	{
		ExtBuffer<char> buff;
		auto doc = remoteType().Field("tp_doc");
		if (doc.GetPtr() == 0)
			return {};

		doc.Dereference().GetString(&buff);
		return buff.GetBuffer();
	}


	auto PyTypeObject::members() const -> vector<unique_ptr<PyMemberDef>>
	{
		vector<unique_ptr<PyMemberDef>> members;
		auto tp_members = remoteType().Field("tp_members");
		auto ptr = tp_members.GetPtr();
		if (ptr != 0) {
			auto isKnownType = false;
			utils::ignoreExtensionError([&] {
				tp_members = ExtRemoteTyped("PyMemberDef", ptr, true);  // necessary for Python >= 3.8
				isKnownType = true;
			});
			if (isKnownType) {
				for (int i = 0; ; ++i) {
					auto elem = tp_members.ArrayElement(i);
					if (elem.Field("name").GetPtr() == 0)
						break;
					members.push_back(make_unique<PyMemberDefAuto>(elem.GetPointerTo().GetPtr()));
				}
			} else {
				// The symbol "PyMemberDef" is not available in Python 3.11.
				// (In Python 3.12 it is available again...)
				auto ptrSize = Layout::pointerSize();
				auto memberDefSize = 5 * ptrSize;
				while (ExtRemoteTyped("(void**)@$extin", ptr).Dereference().GetPtr() != 0) {
					members.push_back(make_unique<PyMemberDefManual>(ptr));
					ptr += memberDefSize;
				}
			}
		}
		return members;
	}


	auto PyTypeObject::isManagedDict() const -> bool
	{
		if (isPython2())  // in Python 2 this bit has another meaning
			return false;
		auto flags_ = remoteType().Field("tp_flags");
		auto flags = utils::readIntegral<unsigned long>(flags_);
		return flags & (1 << 4);  // Py_TPFLAGS_MANAGED_DICT
	}


	auto PyTypeObject::getStaticBuiltinIndex() const -> SSize
	{
		if (isPython2())
			return -1;
		auto type = remoteType();
		auto flagsRaw = type.Field("tp_flags");
		auto flags = utils::readIntegral<unsigned long>(flagsRaw);
		if (flags & (1 << 1)) {  // _Py_TPFLAGS_STATIC_BUILTIN
			// Python >= 3.12
			auto subclassesRaw = type.Field("tp_subclasses");
			auto subclasses = utils::readIntegral<SSize>(subclassesRaw);
			return subclasses - 1;
		}
		return -1;
	}


	auto PyTypeObject::hasInlineValues() const -> bool
	{
		if (isPython2())
			return false;
		auto flagsRaw = remoteType().Field("tp_flags");
		auto flags = utils::readIntegral<unsigned long>(flagsRaw);
		return flags & (1 << 2);  // Py_TPFLAGS_INLINE_VALUES
	}


	auto PyTypeObject::dictOffset() const -> SSize
	{
		auto dictOffset = remoteType().Field("tp_dictoffset");
		return utils::readIntegral<SSize>(dictOffset);
	}


	auto PyTypeObject::mro() const -> unique_ptr<PyTupleObject>
	{
		return utils::fieldAsPyObject<PyTupleObject>(remoteType(), "tp_mro");
	}


	// This isn't really the best place for this method, but it's useful for factories to know
	// which type to construct when a Python3 type differs from a Python 2 type of the same name.
	auto PyTypeObject::isPython2() const -> bool
	{
		return !remoteType().HasField("ob_base");
	}


	auto PyTypeObject::dict() const -> unique_ptr<PyDict>
	{
		Offset dictAddr = 0;
		auto builtinIndex = getStaticBuiltinIndex();
		if (builtinIndex != -1) {
			dictAddr = findStaticTypeDict(currentInterpreterTypes(), builtinIndex, offset());
		} else {
			dictAddr = remoteType().Field("tp_dict").GetPtr();
		}

		if (dictAddr == 0)
			return nullptr;
		return make_unique<PyDictObject>(dictAddr);
	}


	auto PyTypeObject::repr(bool pretty) const -> string
	{
		return linkToSelf("<class '" + name() + "'>", pretty);
	}

}