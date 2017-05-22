#include "PyIntObject.h"

#include "../ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

namespace PyExt::Remote {

	PyIntObject::PyIntObject(Offset objectAddress)
		: PyObject(objectAddress, "PyIntObject")
	{
	}


	auto PyIntObject::intValue() const -> int32_t
	{
		auto ival = remoteType().Field("ob_ival");
		return utils::readIntegral<int32_t>(ival);
	}


	auto PyIntObject::repr(bool /*pretty*/) const -> string
	{
		return to_string(intValue());
	}

}
