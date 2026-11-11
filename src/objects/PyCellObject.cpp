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
		// A cell is empty until its variable is first assigned (and again after `del`).
		auto objPtr = remoteType().Field("ob_ref").GetPtr();
		return objPtr ? make(objPtr) : nullptr;
	}


	auto PyCellObject::repr(bool pretty) const -> string
	{
		string repr = PyObject::repr(pretty);
		auto contents = objectReference();
		if (contents == nullptr)
			return repr + ": " + (pretty ? utils::escapeDml("<empty>") : "<empty>"s);
		return repr + ": " + contents->repr(pretty);
	}

}
