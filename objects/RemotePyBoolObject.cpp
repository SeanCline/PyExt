#include "RemotePyBoolObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyBoolObject::RemotePyBoolObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyBoolObject")
{
}


auto RemotePyBoolObject::boolValue() const -> bool
{
	auto ival = remoteObj().Field("ob_ival");
	return static_cast<bool>(ival.GetLong());
}


auto RemotePyBoolObject::repr(bool /*pretty*/) const -> string
{
	return boolValue() ? "True" : "False";
}
