#include "PySliceObject.h"

#include "../fieldAsPyObject.h"
#include "../ReprBuilder.h"

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
		utils::ReprBuilder out(pretty);
		auto append = [&](const unique_ptr<PyObject>& obj) {
			if (obj != nullptr)
				out.markup(obj->repr(pretty));
			else
				out.text("None");
		};

		out.text("slice(");
		append(start());
		out.text(", ");
		append(stop());
		out.text(", ");
		append(step());
		out.text(")");
		return out.str();
	}

}
