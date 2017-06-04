#include "PySetObject.h"

#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <utils/ScopeExit.h>

#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
using namespace std;

namespace {
	PyExt::Remote::PyObject::Offset getDummyPtr()
	{
		// Dummy is a special pointer used to represent a removed element,
		// distinguished from a never-inserted, NULL element.
		try {
			// Python 3.4+ exports the dummy with a helpful name.
			return (ExtRemoteTyped("_PySet_Dummy").GetPtr());
		} catch (ExtRemoteException&) { /*...*/ }

		try {
			// Python 3.3 exports a single python##!dummy symbol we can use.
			return (ExtRemoteTyped("!dummy").GetPtr());
		} catch (ExtRemoteException&) { /*...*/ }

		// Python 2.7 and earlier have `static PyObject *dummy` variables in multiple translation units.
		// We have no easy way of knowing which is used by PySetObject.
		// As it is, Python 2.7 dumps will display b'<dummy key>' for removed items.
		return 0; //< TODO: Treat all python*!dummy symbols as dummies.
	}
}


namespace PyExt::Remote {

	PySetObject::PySetObject(Offset objectAddress)
		: PyObject(objectAddress, "PySetObject")
	{
	}


	auto PySetObject::numItems() const -> SSize
	{
		auto usedField = remoteType().Field("used");
		return utils::readIntegral<SSize>(usedField);
	}


	auto PySetObject::listValue() const -> vector<unique_ptr<PyObject>>
	{
		vector<unique_ptr<PyObject>> values;

		auto maskField = remoteType().Field("mask");
		auto tableSize = utils::readIntegral<SSize>(maskField) + 1;

		// Dummie entries are used as placeholders for removed elements.
		auto dummyPtr = getDummyPtr();

		for (SSize i = 0; i < tableSize; ++i) {
			auto entry = remoteType().Field("table").ArrayElement(i);
			auto keyPtr = entry.Field("key").GetPtr();
			if (keyPtr == 0 || keyPtr == dummyPtr)
				continue;
			
			auto key = PyObject::make(keyPtr);
			values.push_back(move(key));
		}
		
		return values;
	}


	auto PySetObject::repr(bool pretty) const -> string
	{
		const auto elementSeparator = (pretty) ? "\n" : " "; //< Use a newline when pretty-print is on.
		const auto indentation = (pretty) ? "\t" : ""; //< Indent only when pretty is on.

		ostringstream oss;
		oss << '{' << elementSeparator;

		for (auto& value : listValue()) {
			oss << indentation << value->repr(pretty) << ',' << elementSeparator;
		}

		oss << elementSeparator << '}';
		return oss.str();
	}

}
