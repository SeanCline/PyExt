#include "PyObject.h"

#include "PyTypeObject.h"
#include "PyVarObject.h"
#include "PyDictObject.h"
#include "PyDictKeysObject.h"
#include "../ExtHelpers.h"
#include "../PyLayout.h"
#include "../ReprBuilder.h"

#include <engextcpp.hpp>

#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <optional>
using namespace std;

namespace PyExt::Remote {

	PyObject::PyObject(Offset objectAddress, const string& symbolName /*= "PyObject"*/)
		: RemoteType(objectAddress, symbolName)
	{
	}


	PyObject::~PyObject()
	{
	}


	auto PyObject::refCount() const -> SSize
	{
		// Free-threaded builds split the count into a thread-local part and a shared part,
		// where the low two bits of the shared part are flags.
		if (hasBaseField("ob_ref_local")) {
			auto local = baseField("ob_ref_local");
			auto shared = baseField("ob_ref_shared");
			return static_cast<SSize>(local.GetUlong()) + (utils::readIntegral<SSize>(shared) >> 2);
		}

		// 64-bit Python 3.14+ narrowed ob_refcnt to an unsigned 32-bit field, and immortal objects
		// set its high bits. Reading it as signed would report a large negative count.
		auto refcnt = baseField("ob_refcnt");
		if (refcnt.GetTypeSize() == 4 && Layout::pointerSize() == 8)
			return static_cast<SSize>(refcnt.GetUlong());
		return utils::readIntegral<SSize>(refcnt);
	}


	auto PyObject::isImmortal() const -> bool
	{
		// Mirrors _Py_IsImmortal for each of the layouts Python 3.12+ has used.
		// Older versions have no immortal objects, and can't plausibly reach these counts.
		if (hasBaseField("ob_ref_local"))
			return baseField("ob_ref_local").GetUlong() == 0xFFFFFFFF;

		const auto count = refCount();
		if (Layout::pointerSize() == 8)
			return static_cast<int32_t>(static_cast<uint32_t>(count)) < 0;
		return count >= 0x3FFFFFFF;
	}


	auto PyObject::type() const -> PyTypeObject
	{
		return PyTypeObject(baseField("ob_type").GetPtr());
	}


	auto PyObject::slots() const -> vector<pair<string, unique_ptr<PyObject>>>
	{
		vector<pair<string, unique_ptr<PyObject>>> slots;

		// slots of the type itself and all parents
		for (auto const& typeObj : type().mro()->listValue()) {
			auto members = PyTypeObject(typeObj->offset()).members();
			for (auto const& memberDef : members) {
				// TODO: handle other types than T_OBJECT_EX
				if (memberDef->type() == PyMemberDef::T_OBJECT_EX) {
					auto objPtr = ExtRemoteTyped("(PyObject**)@$extin", offset() + memberDef->offset());
					auto addr = objPtr.Dereference().GetPtr();
					auto value = addr ? make(objPtr.Dereference().GetPtr()) : nullptr;
					slots.emplace_back(memberDef->name(), move(value));
				}
			}
		}
		return slots;
	}


	auto PyObject::managedDict() const -> unique_ptr<PyDict>
	{
		// Python >= 3.11, see PyObject_GenericGetDict
		if (!type().isManagedDict())
			return { };

		auto pointerSize = Layout::pointerSize();

		// Py_TPFLAGS_MANAGED_DICT implies Py_TPFLAGS_PREHEADER, so the allocator
		// always provisions the pre-header region (including the dict pointer)
		// for any type that passes the isManagedDict() guard above.
		bool readManagedDictPointer = false;
		Offset dictPtr = 0;
		utils::ignoreExtensionError([&] {
			// Python >= 3.13
			auto managedDictPtr = offset() + Layout::managedDictSlotOffset();
			dictPtr = ExtRemoteTyped("PyManagedDictPointer", managedDictPtr, true).Field("dict").GetPtr();
			readManagedDictPointer = true;
		});
		if (dictPtr)
			return make_unique<PyDictObject>(dictPtr);

		Offset valuesPtr;
		if (type().hasInlineValues()) {
			// Python >= 3.13: inline values live at offset + tp_basicsize, immediately
			// after any __slots__ storage.
			auto basicSize = static_cast<Offset>(type().basicSize());
			auto dictValues = ExtRemoteTyped("(_dictvalues*)@$extin", offset() + basicSize);
			valuesPtr = dictValues.Field("values").GetPtr();
		} else if (readManagedDictPointer) {
			// Python >= 3.13 without inline values: the managed dict has not been
			// materialized yet (dictPtr was 0 above). Do not fall through to the
			// 3.11/3.12 paths; in 3.13+ the slot at offset - 4*pointerSize is the
			// MANAGED_WEAKREF_OFFSET, not a values pointer, and reading it as such
			// produces a bogus PyManagedDict over (ht_cached_keys, weakref).
			return nullptr;
		} else {
			optional<ExtRemoteTyped> dictOrValues;
			utils::ignoreExtensionError([&] {
				// Python 3.12
				auto dictOrValuesPtr = offset() - 3 * pointerSize;
				dictOrValues = ExtRemoteTyped("PyDictOrValues", dictOrValuesPtr, true);
			});

			if (dictOrValues) {
				valuesPtr = dictOrValues->Field("values").GetPtr();
				if (valuesPtr & 1) {
					valuesPtr += 1;
				} else {
					auto dictFieldPtr = dictOrValues->Field("dict").GetPtr();
					return dictFieldPtr ? make_unique<PyDictObject>(dictFieldPtr) : nullptr;
				}
			} else {
				auto valuesPtrPtr = offset() - 4 * pointerSize;
				valuesPtr = ExtRemoteTyped("(PyObject***)@$extin", valuesPtrPtr).Dereference().GetPtr();
				if (valuesPtr == 0) {
					auto dictFieldPtr = ExtRemoteTyped("(PyDictObject**)@$extin", valuesPtrPtr + pointerSize).Dereference().GetPtr();
					return dictFieldPtr ? make_unique<PyDictObject>(dictFieldPtr) : nullptr;
				}
			}
		}

		auto ht = ExtRemoteTyped("PyHeapTypeObject", type().offset(), true);
		auto cachedKeys = ht.Field("ht_cached_keys");
		return make_unique<PyManagedDict>(cachedKeys.GetPtr(), valuesPtr);
	}


	auto PyObject::dict() const -> unique_ptr<PyDict>
	{
		auto managedDict_ = managedDict();
		if (managedDict_ != nullptr)
			return managedDict_;

		// see https://docs.python.org/3.10/c-api/typeobj.html#c.PyTypeObject.tp_dictoffset
		auto dictOffset_ = type().dictOffset();
		if (dictOffset_ == 0)
			return nullptr;
		if (dictOffset_ < 0) {
			auto thisAsVar = PyVarObject(this->offset());
			auto obSize = thisAsVar.size();
			if (obSize < 0)
				obSize = -obSize;
			dictOffset_ += type().basicSize() + obSize * type().itemSize();
			// alignment
			auto ptrSize = Layout::pointerSize();
			dictOffset_ = (dictOffset_ + (ptrSize - 1)) & ~(ptrSize - 1);
		}
		auto dictPtr = ExtRemoteTyped("(PyDictObject**)@$extin", offset() + dictOffset_);
		auto dictAddr = dictPtr.Dereference().GetPtr();
		return dictAddr ? make_unique<PyDictObject>(dictAddr) : nullptr;
	}


	auto PyObject::repr(bool pretty) const -> string
	{
		return linkToSelf("<" + type().name() + " object>", pretty);
	}


	auto PyObject::linkToSelf(const string& text, bool pretty) const -> string
	{
		return utils::ReprBuilder(pretty).link(text, "!pyobj 0n" + to_string(offset())).str();
	}


	auto PyObject::details() const -> string
	{
		utils::ReprBuilder out(true); //< Details are always printed as DML.
		bool empty = true;
		auto startSection = [&] {
			if (!empty)
				out.text("\n");
			empty = false;
		};

		// repr of built-in base types (not for built-in types itself)
		auto const& types = PyTypeObject::builtinTypes();
		if (find(begin(types), end(types), type().name()) == end(types)) {
			for (auto const& typeObj : type().mro()->listValue()) {
				auto const typeName = PyTypeObject(typeObj->offset()).name();
				if (find(begin(types), end(types), typeName) != end(types)) {
					startSection();
					out.text(typeName + " repr: ").markup(make(offset(), typeName)->repr(true));
				}
			}
		}

		// __slots__ (including base types)
		auto slots_ = slots();
		if (slots_.size() > 0) {
			startSection();
			out.text("slots: {\n");
			for (auto const& [name, value] : slots_) {
				if (value != nullptr)
					out.text("\t" + name + ": ").markup(value->repr(true)).text(",\n");
			}
			out.text("}");
		}

		// __dict__
		auto dictPtr = dict();
		if (dictPtr != nullptr) {
			startSection();
			out.text("dict: ").markup(dictPtr->repr(true));
		}

		return out.str();
	}


	auto PyObject::baseStruct(const string & fieldName) const -> ExtRemoteTyped
	{
		ExtRemoteTyped obj = remoteType();

		// Python3 tucks the base members away in a struct named ob_base.
		// The unicode structs nest theirs in a member named _base instead.
		// Drill down until we find the field or run out of bases.
		while (!obj.HasField(fieldName.c_str())) {
			if (obj.HasField("ob_base"))
				obj = obj.Field("ob_base");
			else if (obj.HasField("_base"))
				obj = obj.Field("_base");
			else
				break;
		}

		return obj;
	}


	auto PyObject::baseField(const string & fieldName) const -> ExtRemoteTyped
	{
		return baseStruct(fieldName).Field(fieldName.c_str());
	}


	auto PyObject::hasBaseField(const string & fieldName) const -> bool
	{
		return baseStruct(fieldName).HasField(fieldName.c_str());
	}

}
