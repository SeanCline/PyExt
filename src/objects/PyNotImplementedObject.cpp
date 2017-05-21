#include "PyNotImplementedObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

namespace PyExt::Remote {

	PyNotImplementedObject::PyNotImplementedObject(Offset objectAddress)
		: PyObject(objectAddress)
	{
	}


	auto PyNotImplementedObject::repr(bool /*pretty*/) const -> string
	{
		return "NotImplemented";
	}

}
