#include "PyEllipsisObject.h"

#include <string>
using namespace std;

namespace PyExt::Remote {

	PyEllipsisObject::PyEllipsisObject(Offset objectAddress)
		: PyObject(objectAddress)
	{
	}


	auto PyEllipsisObject::repr(bool /*pretty*/) const -> string
	{
		return "Ellipsis";
	}

}
