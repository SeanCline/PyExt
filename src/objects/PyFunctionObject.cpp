#include "PyFunctionObject.h"

#include "PyCodeObject.h"
#include "PyDictObject.h"
#include "PyTupleObject.h"
#include "PyDictObject.h"
#include "PyListObject.h"
#include "PyStringObject.h"
#include "../fieldAsPyObject.h"

#include <engextcpp.hpp>

#include <string>
#include <memory>
using namespace std;

namespace PyExt::Remote {

	PyFunctionObject::PyFunctionObject(Offset objectAddress)
		: PyObject(objectAddress, "PyFunctionObject")
	{
	}


	auto PyFunctionObject::code() const -> unique_ptr<PyCodeObject>
	{
		return utils::fieldAsPyObject<PyCodeObject>(remoteType(), "func_code");
	}


	auto PyFunctionObject::globals() const -> unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "func_globals");
	}


	auto PyFunctionObject::defaults() const -> unique_ptr<PyTupleObject>
	{
		return utils::fieldAsPyObject<PyTupleObject>(remoteType(), "func_defaults");
	}


	auto PyFunctionObject::kwdefaults() const -> unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "func_kwdefaults");
	}


	auto PyFunctionObject::closure() const -> unique_ptr<PyTupleObject>
	{
		return utils::fieldAsPyObject<PyTupleObject>(remoteType(), "func_closure");
	}


	auto PyFunctionObject::doc() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "func_doc");
	}


	auto PyFunctionObject::name() const -> unique_ptr<PyStringObject>
	{
		return utils::fieldAsPyObject<PyStringObject>(remoteType(), "func_name");
	}


	auto PyFunctionObject::dict() const -> unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "func_dict");
	}


	auto PyFunctionObject::weakreflist() const -> unique_ptr<PyListObject>
	{
		return utils::fieldAsPyObject<PyListObject>(remoteType(), "func_weakreflist");
	}


	auto PyFunctionObject::module() const -> unique_ptr<PyObject>
	{
		return utils::fieldAsPyObject<PyObject>(remoteType(), "func_module");
	}


	auto PyFunctionObject::annotations() const -> unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "func_annotations");
	}


	auto PyFunctionObject::qualname() const -> unique_ptr<PyStringObject>
	{
		return utils::fieldAsPyObject<PyStringObject>(remoteType(), "func_qualname");
	}


	auto PyFunctionObject::repr(bool /*pretty*/) const -> string
	{
		auto nameObject = name();
		if (nameObject == nullptr)
			return "<function>";

		return "<function " + nameObject->stringValue() + ">";
	}

}
