#include "PySuperObject.h"

#include "PyTypeObject.h"
#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"
#include "../ReprBuilder.h"

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
		utils::ReprBuilder out(pretty);
		out.text("<super: ");

		if (auto thisClassObj = thisClass())
			out.markup(thisClassObj->repr(pretty));
		else
			out.text("NULL");

		auto objType = objectType();
		out.text(", " + (objType != nullptr ? "<" + objType->name() + " object>" : "NULL"s) + ">");
		return out.str();
	}

}
