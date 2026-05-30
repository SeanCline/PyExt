#include "PyObject.h"

#include "PyBuild.h"
#include "PyTypeObject.h"
#include "PyVarObject.h"
#include "PyDictObject.h"
#include "PyDictKeysObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <cstdint>
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
		if (isFreeThreaded()) {
			// PEP 703: refcount is split into a thread-owned uint32 plus an atomic
			// shared int64. The low _Py_REF_SHARED_SHIFT (=2) bits of ob_ref_shared
			// are flag/state bits; the rest is the shared count.
			auto local = baseField("ob_ref_local");
			auto shared = baseField("ob_ref_shared");
			auto localVal = utils::readIntegral<uint32_t>(local);
			auto sharedVal = utils::readIntegral<int64_t>(shared);
			return static_cast<SSize>(localVal) + static_cast<SSize>(sharedVal >> 2);
		}

		auto refcnt = baseField("ob_refcnt");
		// Python 3.14+: ob_refcnt shrank from Py_ssize_t (8 bytes) to uint32_t
		// (4 bytes), with the high bit signalling immortality. Read it unsigned
		// so an immortal singleton widens to a large positive number rather than
		// sign-extending to a giant negative one.
		if (refcnt.GetTypeSize() == 4)
			return static_cast<SSize>(utils::readIntegral<uint32_t>(refcnt));
		return utils::readIntegral<SSize>(refcnt);
	}


	auto PyObject::headerSize() -> SSize
	{
		// `_Py_NoneStruct` is the None singleton, statically typed as PyObject,
		// so its size is sizeof(struct _object) — which doubles on the
		// free-threaded build (the extra ob_tid/ob_mutex/ob_ref_local/etc.
		// fields). Queried each call; the underlying GetTypeSize is metadata
		// only and stays cheap.
		try {
			return static_cast<SSize>(ExtRemoteTyped("_Py_NoneStruct").GetTypeSize());
		} catch (...) {
			// Fall back to the GIL-build size if the None singleton symbol is
			// somehow unavailable — better than returning zero and producing
			// silently bogus address arithmetic downstream.
			return 2 * static_cast<SSize>(utils::getPointerSize());
		}
	}


	auto PyObject::managedDictOffset() -> SSize
	{
		auto pointerSize = static_cast<SSize>(utils::getPointerSize());
		// Pre-header layout (CPython Include/internal/pycore_object.h):
		//   GIL build:           [-4*ptr weakref] [-3*ptr managed_dict] [obj]
		//   free-threaded build: [-2*ptr weakref] [-1*ptr managed_dict] [obj]
		return isFreeThreaded() ? -pointerSize : -3 * pointerSize;
	}


	auto PyObject::isImmortal() const -> bool
	{
		if (isFreeThreaded()) {
			// _Py_IMMORTAL_REFCNT_LOCAL == UINT32_MAX.
			auto local = baseField("ob_ref_local");
			return utils::readIntegral<uint32_t>(local) == 0xFFFFFFFFu;
		}

		auto refcnt = baseField("ob_refcnt");
		// _Py_IsImmortal: top bit of the uint32 ob_refcnt (3.14+). Pre-3.14
		// ob_refcnt is Py_ssize_t and has no immortality bit at this layout.
		if (refcnt.GetTypeSize() == 4)
			return (utils::readIntegral<uint32_t>(refcnt) & 0x80000000u) != 0;
		return false;
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

		auto pointerSize = utils::getPointerSize();

		// Py_TPFLAGS_MANAGED_DICT implies Py_TPFLAGS_PREHEADER, so the allocator
		// always provisions the pre-header region (including the managed-dict
		// slot) for any type that passes isManagedDict(). The slot's offset
		// depends on build mode — see managedDictOffset().
		bool readManagedDictPointer = false;
		Offset dictPtr = 0;
		utils::ignoreExtensionError([&] {
			// Python >= 3.13
			auto managedDictPtr = offset() + managedDictOffset();
			dictPtr = ExtRemoteTyped("PyManagedDictPointer", managedDictPtr, true).Field("dict").GetPtr();
			readManagedDictPointer = true;
		});
		if (dictPtr)
			return make_unique<PyDictObject>(dictPtr);

		Offset valuesPtr;
		if (type().hasInlineValues()) {
			// Python >= 3.13: inline values live at offset + tp_basicsize, immediately
			// after any __slots__ storage. Adjust the @$extin base so the expression
			// "((PyObject*)(@$extin)+1)" — which DbgEng evaluates by adding
			// sizeof(PyObject) — lands at offset() + basicSize. Use headerSize()
			// because sizeof(PyObject) doubles on the free-threaded build.
			auto basicSize = static_cast<Offset>(type().basicSize());
			auto adjustedBase = offset() + basicSize - static_cast<Offset>(headerSize());
			auto dictValues = ExtRemoteTyped("(_dictvalues*)((PyObject*)(@$extin)+1)", adjustedBase);
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
			auto ptrSize = utils::getPointerSize();
			dictOffset_ = (dictOffset_ + (ptrSize - 1)) & ~(ptrSize - 1);
		}
		auto dictPtr = ExtRemoteTyped("(PyDictObject**)@$extin", offset() + dictOffset_);
		auto dictAddr = dictPtr.Dereference().GetPtr();
		return dictAddr ? make_unique<PyDictObject>(dictAddr) : nullptr;
	}


	auto PyObject::repr(bool pretty) const -> string
	{
		string repr = "<" + type().name() + " object>";
		if (pretty)
			return utils::link(repr, "!pyobj 0n"s + to_string(offset()));
		return repr;
	}


	auto PyObject::details() const -> string
	{
		const auto sectionSeparator = "\n";
		const auto elementSeparator = "\n";
		const auto indentation = "\t";
		ostringstream oss;
		bool empty = true;

		// repr of built-in base types (not for built-in types itself)
		auto const& types = PyTypeObject::builtinTypes();
		if (std::find(begin(types), end(types), type().name()) == types.end()) {
			for (auto const& typeObj : type().mro()->listValue()) {
				auto const typeName = PyTypeObject(typeObj->offset()).name();
				if (find(begin(types), end(types), typeName) != end(types)) {
					if (!empty)
						oss << sectionSeparator;
					else
						empty = false;
					oss << typeName << " repr: " << make(offset(), typeName)->repr();
				}
			}
		}

		// __slots__ (including base types)
		auto slots_ = slots();
		if (slots_.size() > 0) {
			if (!empty)
				oss << sectionSeparator;
			else
				empty = false;
			oss << "slots: {" << elementSeparator;
			for (auto const& [name, value] : slots()) {
				if (value != nullptr)
					oss << indentation << name << ": " << value->repr(true) << ',' << elementSeparator;
			}
			oss << '}';
		}

		// __dict__
		auto dictPtr = dict();
		if (dictPtr != nullptr) {
			if (!empty)
				oss << sectionSeparator;
			else
				empty = false;
			oss << "dict: " << dictPtr->repr(true);
		}

		return oss.str();
	}


	auto PyObject::baseField(const string & fieldName) const -> ExtRemoteTyped
	{
		ExtRemoteTyped obj = remoteType();

		// Python3 tucks the base members away in a struct named ob_base.
		while (obj.HasField("ob_base") && !obj.HasField(fieldName.c_str())) {
			// Drill down into the ob_base member until we hit the end.
			obj = obj.Field("ob_base");
		}

		return obj.Field(fieldName.c_str());
	}

}
