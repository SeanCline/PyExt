#include "PyObject.h"

#include "PyTypeObject.h"
#include "PyVarObject.h"
#include "PyDictObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
#include <sstream>
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
		auto refcnt = baseField("ob_refcnt");
		return utils::readIntegral<SSize>(refcnt);
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
					slots.push_back(make_pair(memberDef->name(), move(value)));
				}
			}
		}
		return slots;
	}


	auto PyObject::dict() const -> unique_ptr<PyDictObject>
	{
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
			dictOffset_ = (dictOffset_ + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1);
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

		// repr of built-in base types
		for (auto const& typeObj : type().mro()->listValue()) {
			auto const typeName = PyTypeObject(typeObj->offset()).name();
			auto const& types = PyTypeObject::builtinTypes;
			if (find(types.begin(), types.end(), typeName) != types.end()) {
				if (!empty)
					oss << sectionSeparator;
				else
					empty = false;
				oss << typeName << " repr: " << make(offset(), typeName)->repr();
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
			for (auto const& pairValue : slots()) {
				auto const& name = pairValue.first;
				auto const& value = pairValue.second;
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
