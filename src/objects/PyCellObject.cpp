#include "PyCellObject.h"

#include "PyTypeObject.h"

#include "../ExtHelpers.h"

#include <string>
using namespace std;


namespace PyExt::Remote {

	PyCellObject::PyCellObject(Offset objectAddress)
		: PyObject(objectAddress, "PyCellObject")
	{
	}


	auto PyCellObject::objectReference() const -> unique_ptr<PyObject>
	{
		auto objPtr = remoteType().Field("ob_ref").GetPtr();
		return make(objPtr);
	}


	auto PyCellObject::repr(bool pretty) const -> string
	{
		string repr = PyObject::repr(pretty);
		return repr + ": " + objectReference()->repr(pretty);
	}

}
