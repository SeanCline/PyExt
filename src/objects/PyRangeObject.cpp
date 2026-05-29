#include "PyRangeObject.h"

#include "../fieldAsPyObject.h"

#include <string>
using namespace std;

namespace PyExt::Remote {

	PyRangeObject::PyRangeObject(Offset objectAddress)
		: PyObject(objectAddress, "rangeobject")
	{
	}


	auto PyRangeObject::start() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "start");
	}


	auto PyRangeObject::stop() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "stop");
	}


	auto PyRangeObject::step() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "step");
	}


	auto PyRangeObject::length() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "length");
	}


	auto PyRangeObject::repr(bool pretty) const -> string
	{
		auto startObj = start();
		auto stopObj = stop();
		auto stepObj = step();
		string s = startObj ? startObj->repr(pretty) : "?";
		string e = stopObj ? stopObj->repr(pretty) : "?";
		string st = stepObj ? stepObj->repr(pretty) : "?";
		// Match CPython: range omits the step if it is exactly 1.
		if (stepObj && stepObj->repr(false) == "1")
			return "range(" + s + ", " + e + ")";
		return "range(" + s + ", " + e + ", " + st + ")";
	}

}
