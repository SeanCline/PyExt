#include "PyFloatObject.h"

#include <engextcpp.hpp>
#include <format>
#include <string>
using namespace std;

namespace PyExt::Remote {

	PyFloatObject::PyFloatObject(Offset objectAddress)
		: PyObject(objectAddress, "PyFloatObject")
	{
	}


	auto PyFloatObject::floatValue() const -> double
	{
		auto fval = remoteType().Field("ob_fval");
		return fval.GetDouble();
	}


	auto PyFloatObject::repr(bool /*pretty*/) const -> string
	{
		// std::format gives the shortest representation that round-trips, like Python's repr().
		// std::to_string would use a fixed six decimals, turning 1.5e-10 into "0.000000".
		auto repr = format("{}", floatValue());

		// Python always marks floats as such, e.g. "1.0" rather than "1".
		if (repr.find_first_not_of("-0123456789") == string::npos)
			repr += ".0";
		return repr;
	}

}
