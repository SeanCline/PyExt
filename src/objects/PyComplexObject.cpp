#include "PyComplexObject.h"

#include <engextcpp.hpp>
#include <string>
#include <complex>
#include <format>
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
		// signbit rather than `< 0` so that a negative zero keeps its sign, as it does in Python.
		string operation = signbit(c.imag()) ? "-" : "+";
		// Shortest round-trip formatting. std::to_string would use a fixed six decimals.
		return "(" + format("{}", c.real()) + operation + format("{}", fabs(c.imag())) + "j)";
	}

}