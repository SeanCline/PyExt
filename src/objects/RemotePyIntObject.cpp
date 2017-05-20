#include "RemotePyIntObject.h"

#include "utils/ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyIntObject::RemotePyIntObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyIntObject")
{
}


auto RemotePyIntObject::intValue() const -> int32_t
{
	auto ival = remoteObj().Field("ob_ival");
	return utils::readIntegral<int32_t>(ival);
}


auto RemotePyIntObject::repr(bool /*pretty*/) const -> string
{
	return to_string(intValue());
}
