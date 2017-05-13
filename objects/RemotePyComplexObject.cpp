#include "RemotePyComplexObject.h"

#include <engextcpp.hpp>
#include <string>
#include <complex>
#include <cmath>
using namespace std;

RemotePyComplexObject::RemotePyComplexObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyComplexObject")
{
}


auto RemotePyComplexObject::complexValue() const -> complex<double>
{
	auto cval = remoteObj().Field("cval");
	return { cval.Field("real").GetDouble(), cval.Field("imag").GetDouble() };
}


auto RemotePyComplexObject::repr(bool /*pretty*/) const -> string
{
	auto c = complexValue();
	string operation = (c.imag() < 0) ? "-" : "+";
	return "(" + to_string(c.real()) + operation + to_string(fabs(c.imag())) + "j)";
}
