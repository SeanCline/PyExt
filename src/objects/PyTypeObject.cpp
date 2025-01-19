#include "PyTypeObject.h"

#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

namespace PyExt::Remote {
	auto PyTypeObject::builtinTypes() -> const std::vector<std::string>&
	{
		static const std::vector<std::string> types {
			"type",
			"str",
			"bytes",
			"bytearray",
			"tuple",
			"set",
			"dict",
			"int",
			"long",
			"float",
			"bool",
			"complex",
			"frame",
			"code",
			"function",
			"cell",
			"NoneType",
			"NotImplementedType",
		};

		return types;
	}

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
		auto doc = remoteType().Field("ob_type").Field("tp_doc");
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
				auto ptrSize = utils::getPointerSize();
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
		Offset dictAddr;
		auto builtinIndex = getStaticBuiltinIndex();
		if (builtinIndex != -1) {
			auto builtins = ExtRemoteTyped("_PyRuntime.interpreters.main->types.builtins");  // TODO: Support multiple interpreters
			if (builtins.HasField("initialized"))  // Python >= 3.13
				builtins = builtins.Field("initialized");
			auto builtinState = builtins.ArrayElement(builtinIndex);
			dictAddr = builtinState.Field("tp_dict").GetPtr();
		} else {
			dictAddr = remoteType().Field("tp_dict").GetPtr();
		}
		return make_unique<PyDictObject>(dictAddr);
	}


	auto PyTypeObject::repr(bool pretty) const -> string
	{
		string repr = "<class '" + name() + "'>";
		if (pretty)
			return utils::link(repr, "!pyobj 0n"s + to_string(offset()));
		return repr;
	}

}