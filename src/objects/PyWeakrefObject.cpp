#include "PyWeakrefObject.h"

#include "PyTypeObject.h"
#include "../fieldAsPyObject.h"
#include "../ReprBuilder.h"

#include <string>
using namespace std;

namespace PyExt::Remote {

	PyWeakrefObject::PyWeakrefObject(Offset objectAddress)
		: PyObject(objectAddress, "_PyWeakReference")
	{
	}


	auto PyWeakrefObject::referent() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "wr_object");
	}


	auto PyWeakrefObject::callback() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "wr_callback");
	}


	auto PyWeakrefObject::repr(bool pretty) const -> string
	{
		// A dead weakref has wr_object replaced with Py_None (not NULL), so a null check
		// alone isn't enough: match on the referent's type name.
		auto referentObj = referent();
		utils::ReprBuilder out(pretty);
		if (referentObj == nullptr || referentObj->type().name() == "NoneType")
			out.text("<" + type().name() + " dead>");
		else
			out.text("<" + type().name() + " to ").markup(referentObj->repr(pretty)).text(">");
		return out.str();
	}

}
