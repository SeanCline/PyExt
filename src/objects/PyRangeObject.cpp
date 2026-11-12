#include "PyRangeObject.h"

#include "../fieldAsPyObject.h"
#include "../ReprBuilder.h"

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
		utils::ReprBuilder out(pretty);
		auto append = [&](const unique_ptr<PyObject>& obj) {
			if (obj != nullptr)
				out.markup(obj->repr(pretty));
			else
				out.text("?");
		};

		out.text("range(");
		append(start());
		out.text(", ");
		append(stop());

		// Match CPython: range omits the step if it is exactly 1.
		auto stepObj = step();
		if (stepObj == nullptr || stepObj->repr(false) != "1") {
			out.text(", ");
			append(stepObj);
		}

		out.text(")");
		return out.str();
	}

}
