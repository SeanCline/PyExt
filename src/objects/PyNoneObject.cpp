#include "PyNoneObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

namespace PyExt::Remote {

	PyNoneObject::PyNoneObject(Offset objectAddress)
		: PyObject(objectAddress)
	{
	}


	auto PyNoneObject::repr(bool /*pretty*/) const -> string
	{
		return "None";
	}

}