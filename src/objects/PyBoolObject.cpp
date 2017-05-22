#include "PyBoolObject.h"

#include "../ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;


namespace PyExt::Remote {

	PyBoolObject::PyBoolObject(Offset objectAddress)
		: PyObject(objectAddress, "PyBoolObject")
	{
	}


	auto PyBoolObject::boolValue() const -> bool
	{
		auto ival = remoteType().Field("ob_ival");
		return utils::readIntegral<bool>(ival);
	}


	auto PyBoolObject::repr(bool /*pretty*/) const -> string
	{
		return boolValue() ? "True" : "False";
	}

}
