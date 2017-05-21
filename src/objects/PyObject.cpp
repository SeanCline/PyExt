#include "PyObject.h"

#include "PyTypeObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
using namespace std;

namespace PyExt::Remote {

	PyObject::PyObject(Offset objectAddress, const string& symbolName /*= "PyObject"*/)
		: remoteObj_(make_shared<ExtRemoteTyped>(symbolName.c_str(), objectAddress, true)),
		symbolName_(symbolName)
	{
	}


	PyObject::~PyObject()
	{
	}


	PyObject::PyObject(const PyObject&) = default;
	PyObject& PyObject::operator=(const PyObject&) = default;
	PyObject::PyObject(PyObject&&) = default;
	PyObject& PyObject::operator=(PyObject&&) = default;


	auto PyObject::offset() const -> Offset
	{
		return remoteObj().GetPtr();
	}


	auto PyObject::symbolName() const -> std::string
	{
		return symbolName_;
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


	auto PyObject::repr(bool /*pretty*/) const -> string
	{
		return "<" + type().name() + " object>";
	}


	auto PyObject::remoteObj() const -> ExtRemoteTyped&
	{
		return *remoteObj_;
	}


	auto PyObject::baseField(const string & fieldName) const -> ExtRemoteTyped
	{
		ExtRemoteTyped obj = remoteObj();

		// Python3 tucks the base members away in a struct named ob_base.
		while (obj.HasField("ob_base") && !obj.HasField(fieldName.c_str())) {
			// Drill down into the ob_base member until we hit the end.
			obj = obj.Field("ob_base");
		}

		return obj.Field(fieldName.c_str());
	}

}
