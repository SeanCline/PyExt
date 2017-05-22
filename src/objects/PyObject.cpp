#include "PyObject.h"

#include "PyTypeObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
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


	auto PyObject::repr(bool /*pretty*/) const -> string
	{
		return "<" + type().name() + " object>";
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
