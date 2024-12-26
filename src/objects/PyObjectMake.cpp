#include "PyObject.h"
#include "PyVarObject.h"
#include "PyTypeObject.h"
#include "PyStringObject.h"
#include "PyUnicodeObject.h"
#include "PyByteArrayObject.h"
#include "PyListObject.h"
#include "PyTupleObject.h"
#include "PySetObject.h"
#include "PyDictObject.h"
#include "PyIntObject.h"
#include "PyLongObject.h"
#include "PyFloatObject.h"
#include "PyBoolObject.h"
#include "PyComplexObject.h"
#include "PyFrameObject.h"
#include "PyCodeObject.h"
#include "PyFunctionObject.h"
#include "PyCellObject.h"
#include "PyNoneObject.h"
#include "PyNotImplementedObject.h"

#include <memory>
using namespace std;

namespace PyExt::Remote {

	auto PyObject::make(PyObject::Offset remoteAddress) -> unique_ptr<PyObject>
	{
		// Get the type of this object.
		const auto typeObj = PyObject(remoteAddress).type();
		const auto typeName = typeObj.name();

		return make(remoteAddress, typeName);
	}


	auto PyObject::make(PyObject::Offset remoteAddress, const std::string& typeName) -> unique_ptr<PyObject>
	{
		// TODO: Turn this into a map to factory functions.
		if (typeName == "type") {
			return make_unique<PyTypeObject>(remoteAddress);
		} else if (typeName == "str") {
			const auto typeObj = PyObject(remoteAddress).type();
			if (typeObj.isPython2()) {
				return make_unique<PyStringObject>(remoteAddress);
			} else {
				return make_unique<PyUnicodeObject>(remoteAddress);
			}
		} else if (typeName == "bytes") {
			return make_unique<PyBytesObject>(remoteAddress);
		} else if (typeName == "bytearray") {
			return make_unique<PyByteArrayObject>(remoteAddress);
		} else if (typeName == "list") {
			return make_unique<PyListObject>(remoteAddress);
		} else if (typeName == "tuple") {
			return make_unique<PyTupleObject>(remoteAddress);
		} else if (typeName == "set") {
			return make_unique<PySetObject>(remoteAddress);
		} else if (typeName == "dict") {
			return make_unique<PyDictObject>(remoteAddress);
		} else if (typeName == "int") {
			const auto typeObj = PyObject(remoteAddress).type();
			if (typeObj.isPython2()) {
				return make_unique<PyIntObject>(remoteAddress);
			} else {
				return make_unique<PyLongObject>(remoteAddress);
			}
		} else if (typeName == "long") {
			return make_unique<PyLongObject>(remoteAddress);
		} else if (typeName == "float") {
			return make_unique<PyFloatObject>(remoteAddress);
		} else if (typeName == "bool") {
			const auto typeObj = PyObject(remoteAddress).type();
			if (typeObj.isPython2()) {
				return make_unique<PyBoolObject>(remoteAddress);
			} else {
				return make_unique<PyLongObject>(remoteAddress, true);
			}
		} else if (typeName == "complex") {
			return make_unique<PyComplexObject>(remoteAddress);
		} else if (typeName == "frame") {
			return make_unique<PyFrameObject>(remoteAddress);
		} else if (typeName == "code") {
			return make_unique<PyCodeObject>(remoteAddress);
		} else if (typeName == "function") {
			return make_unique<PyFunctionObject>(remoteAddress);
		} else if (typeName == "cell") {
			return make_unique<PyCellObject>(remoteAddress);
		} else if (typeName == "NoneType") {
			return make_unique<PyNoneObject>(remoteAddress);
		} else if (typeName == "NotImplementedType") {
			return make_unique<PyNotImplementedObject>(remoteAddress);
		} else {
			return make_unique<PyObject>(remoteAddress);
		}
	}

}
