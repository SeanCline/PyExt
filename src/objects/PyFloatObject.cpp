#include "PyFloatObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

namespace PyExt::Remote {

	PyFloatObject::PyFloatObject(Offset objectAddress)
		: PyObject(objectAddress, "PyFloatObject")
	{
	}


	auto PyFloatObject::floatValue() const -> double
	{
		auto fval = remoteObj().Field("ob_fval");
		return fval.GetDouble();
	}


	auto PyFloatObject::repr(bool /*pretty*/) const -> string
	{
		return to_string(floatValue());
	}

}
