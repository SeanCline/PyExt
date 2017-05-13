#include "RemotePyFloatObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyFloatObject::RemotePyFloatObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyFloatObject")
{
}


double RemotePyFloatObject::floatValue() const
{
	auto fval = remoteObj().Field("ob_fval");
	return fval.GetDouble();
}


string RemotePyFloatObject::repr(bool /*pretty*/) const
{
	return to_string(floatValue());
}
