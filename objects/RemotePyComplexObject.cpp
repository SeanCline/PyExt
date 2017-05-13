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


complex<double> RemotePyComplexObject::complexValue() const
{
	auto cval = remoteObj().Field("cval");
	return { cval.Field("real").GetDouble(), cval.Field("imag").GetDouble() };
}


string RemotePyComplexObject::repr(bool /*pretty*/) const
{
	auto c = complexValue();
	string operation = (c.imag() < 0) ? "-" : "+";
	return "(" + to_string(c.real()) + operation + to_string(fabs(c.imag())) + "j)";
}
