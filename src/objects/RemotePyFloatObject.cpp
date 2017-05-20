#include "RemotePyFloatObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyFloatObject::RemotePyFloatObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyFloatObject")
{
}


auto RemotePyFloatObject::floatValue() const -> double
{
	auto fval = remoteObj().Field("ob_fval");
	return fval.GetDouble();
}


auto RemotePyFloatObject::repr(bool /*pretty*/) const -> string
{
	return to_string(floatValue());
}
