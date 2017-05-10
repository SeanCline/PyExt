#include "RemotePyBoolObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyBoolObject::RemotePyBoolObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyBoolObject")
{
}


bool RemotePyBoolObject::boolValue() const
{
	auto ival = remoteObj().Field("ob_ival");
	return static_cast<bool>(ival.GetLong());
}


string RemotePyBoolObject::repr(bool pretty) const
{
	return boolValue() ? "True" : "False";
}
