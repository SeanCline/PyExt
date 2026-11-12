#include "PyCellObject.h"

#include "PyTypeObject.h"

#include "../ExtHelpers.h"
#include "../ReprBuilder.h"

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
		utils::ReprBuilder out(pretty);
		out.markup(PyObject::repr(pretty)).text(": ");

		auto contents = objectReference();
		if (contents == nullptr)
			out.text("<empty>");
		else
			out.markup(contents->repr(pretty));
		return out.str();
	}

}
