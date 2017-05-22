#include "PyComplexObject.h"

#include <engextcpp.hpp>
#include <string>
#include <complex>
#include <cmath>
using namespace std;

namespace PyExt::Remote {

	PyComplexObject::PyComplexObject(Offset objectAddress)
		: PyObject(objectAddress, "PyComplexObject")
	{
	}


	auto PyComplexObject::complexValue() const -> complex<double>
	{
		auto cval = remoteType().Field("cval");
		return { cval.Field("real").GetDouble(), cval.Field("imag").GetDouble() };
	}


	auto PyComplexObject::repr(bool /*pretty*/) const -> string
	{
		auto c = complexValue();
		string operation = (c.imag() < 0) ? "-" : "+";
		return "(" + to_string(c.real()) + operation + to_string(fabs(c.imag())) + "j)";
	}

}