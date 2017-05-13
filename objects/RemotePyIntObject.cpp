#include "RemotePyIntObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyIntObject::RemotePyIntObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyIntObject")
{
}


int32_t RemotePyIntObject::intValue() const
{
	auto ival = remoteObj().Field("ob_ival");
	return ival.GetLong();
}


string RemotePyIntObject::repr(bool /*pretty*/) const
{
	return to_string(intValue());
}
