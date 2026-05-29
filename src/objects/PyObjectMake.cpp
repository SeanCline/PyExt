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
#include "PyEllipsisObject.h"
#include "PySliceObject.h"
#include "PyRangeObject.h"
#include "PySuperObject.h"
#include "PyWeakrefObject.h"
#include "PyMemoryViewObject.h"
#include "PyGenObject.h"

#include <map>
#include <memory>
#include <string>
#include <vector>
using namespace std;

namespace PyExt::Remote {

	namespace {

		// Each entry knows how to build a concrete PyObject subclass from an address.
		using Factory = unique_ptr<PyObject>(*)(PyObject::Offset);


		template <typename T, typename... Ts>
		auto makeAs(PyObject::Offset addr, Ts... ts) -> unique_ptr<PyObject>
		{
			return make_unique<T>(addr, ts...);
		}


		// Aliases for type names whose factory needs to inspect the Python version.
		auto makeStr(PyObject::Offset addr) -> unique_ptr<PyObject>
		{
			return (PyObject(addr).type().isPython2()) ? makeAs<PyStringObject>(addr) : makeAs<PyUnicodeObject>(addr);
		}

		auto makeInt(PyObject::Offset addr) -> unique_ptr<PyObject>
		{
			return (PyObject(addr).type().isPython2()) ? makeAs<PyIntObject>(addr) : makeAs<PyLongObject>(addr);
		}

		auto makeBool(PyObject::Offset addr) -> unique_ptr<PyObject>
		{
			return (PyObject(addr).type().isPython2()) ? makeAs<PyBoolObject>(addr) : makeAs<PyLongObject>(addr, /*isBool=*/ true);
		}


		// Table of factory functions to construct a Python type by its name.
		// PyObject::make()and PyTypeObject::builtinTypes() both use this table.
		const map<string, Factory> factoryTable {
			{ "NoneType",                    &makeAs<PyNoneObject>},
			{ "NotImplementedType",          &makeAs<PyNotImplementedObject>},
			{ "async_generator",             &makeAs<PyAsyncGenObject>},
			{ "bool",                        &makeBool},
			{ "bytearray",                   &makeAs<PyByteArrayObject>},
			{ "bytes",                       &makeAs<PyBytesObject>},
			{ "cell",                        &makeAs<PyCellObject>},
			{ "code",                        &makeAs<PyCodeObject>},
			{ "complex",                     &makeAs<PyComplexObject>},
			{ "coroutine",                   &makeAs<PyCoroObject>},
			{ "dict",                        &makeAs<PyDictObject>},
			{ "ellipsis",                    &makeAs<PyEllipsisObject>},
			{ "float",                       &makeAs<PyFloatObject>},
			{ "frame",                       &makeAs<PyFrameObject>},
			{ "frozendict",                  &makeAs<PyDictObject>}, //< Python 3.15+, shares layout with dict.
			{ "frozenset",                   &makeAs<PySetObject>}, //< Shares layout with set.
			{ "function",                    &makeAs<PyFunctionObject>},
			{ "generator",                   &makeAs<PyGenObject>},
			{ "int",                         &makeInt},
			{ "list",                        &makeAs<PyListObject>},
			{ "long",                        &makeAs<PyLongObject>}, //< Python 2 only.
			{ "memoryview",                  &makeAs<PyMemoryViewObject>},
			{ "range",                       &makeAs<PyRangeObject>},
			{ "set",                         &makeAs<PySetObject>},
			{ "slice",                       &makeAs<PySliceObject>},
			{ "str",                         &makeStr},
			{ "super",                       &makeAs<PySuperObject>},
			{ "tuple",                       &makeAs<PyTupleObject>},
			{ "type",                        &makeAs<PyTypeObject>},
			{ "weakref",                     &makeAs<PyWeakrefObject>}, //< Python <3.9 short name.
			{ "weakref.CallableProxyType",   &makeAs<PyWeakrefObject>},
			{ "weakref.ProxyType",           &makeAs<PyWeakrefObject>},
			{ "weakref.ReferenceType",       &makeAs<PyWeakrefObject>},
		};

	}


	auto PyObject::make(PyObject::Offset remoteAddress) -> unique_ptr<PyObject>
	{
		const auto typeName = PyObject(remoteAddress).type().name(); //< Get the type of this object.
		return make(remoteAddress, typeName);
	}


	auto PyObject::make(PyObject::Offset remoteAddress, const std::string& typeName) -> unique_ptr<PyObject>
	{
		auto it = factoryTable.find(typeName);
		if (it == factoryTable.end())
			return makeAs<PyObject>(remoteAddress); //< Fallback on PyObject.
		return it->second(remoteAddress);
	}


	auto PyTypeObject::builtinTypes() -> const std::vector<std::string>&
	{
		// Derived from the dispatch table to keep the two lists in sync.
		static const std::vector<std::string> types = [] { //< Cache the result so we don't have to rebuild the vector.
			std::vector<std::string> result;
			for (auto const& [name, factory] : factoryTable)
				result.push_back(name);
			return result;
		}();
		return types;
	}

}
