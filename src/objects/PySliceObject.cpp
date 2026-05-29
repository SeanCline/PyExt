#include "PySliceObject.h"

#include "../fieldAsPyObject.h"

#include <string>
using namespace std;

namespace PyExt::Remote {

	PySliceObject::PySliceObject(Offset objectAddress)
		: PyObject(objectAddress, "PySliceObject")
	{
	}


	auto PySliceObject::start() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "start");
	}


	auto PySliceObject::stop() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "stop");
	}


	auto PySliceObject::step() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "step");
	}


	auto PySliceObject::repr(bool pretty) const -> string
	{
		auto reprOrNone = [pretty](const unique_ptr<PyObject>& obj) {
			return obj == nullptr ? "None"s : obj->repr(pretty);
		};
		return "slice(" + reprOrNone(start()) + ", " + reprOrNone(stop()) + ", " + reprOrNone(step()) + ")";
	}

}
