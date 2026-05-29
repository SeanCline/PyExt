#include "PySuperObject.h"

#include "PyTypeObject.h"
#include "../fieldAsPyObject.h"

#include <string>
using namespace std;

namespace PyExt::Remote {

	PySuperObject::PySuperObject(Offset objectAddress)
		: PyObject(objectAddress, "superobject")
	{
	}


	auto PySuperObject::thisClass() const -> unique_ptr<PyTypeObject>
	{
		return utils::fieldAsPyObject<PyTypeObject>(remoteType(), "type");
	}


	auto PySuperObject::thisObject() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "obj");
	}


	auto PySuperObject::objectType() const -> unique_ptr<PyTypeObject>
	{
		return utils::fieldAsPyObject<PyTypeObject>(remoteType(), "obj_type");
	}


	auto PySuperObject::repr(bool pretty) const -> string
	{
		auto typeRepr = thisClass() ? thisClass()->repr(pretty) : "NULL"s;
		auto objType = objectType();
		string objPart;
		if (objType != nullptr)
			objPart = "<" + objType->name() + " object>";
		else
			objPart = "NULL";
		return "<super: " + typeRepr + ", " + objPart + ">";
	}

}
