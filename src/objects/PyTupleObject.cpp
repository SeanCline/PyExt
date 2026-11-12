#include "PyTupleObject.h"

#include "utils/lossless_cast.h"
#include "../ReprBuilder.h"

#include <engextcpp.hpp>

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
using namespace std;

namespace PyExt::Remote {

	PyTupleObject::PyTupleObject(Offset objectAddress)
		: PyVarObject(objectAddress, "PyTupleObject")
	{
	}


	auto PyTupleObject::numItems() const -> SSize
	{
		return PyVarObject::size();
	}


	auto PyTupleObject::at(SSize index) const -> unique_ptr<PyObject>
	{
		if (index < 0 || index >= numItems())
			throw out_of_range("PyTupleObject::at index out of range.");

		auto obj = remoteType();
		auto itemPtr = obj.Field("ob_item").ArrayElement(index).GetPtr();
		return PyObject::make(itemPtr);
	}


	auto PyTupleObject::listValue() const -> vector<unique_ptr<PyObject>>
	{
		auto count = utils::lossless_cast<size_t>(numItems());
		vector<unique_ptr<PyObject>> values(count);

		for (size_t i = 0; i < count; ++i) {
			values[i] = at(i);
		}

		return values;
	}


	auto PyTupleObject::repr(bool pretty) const -> string
	{
		utils::ReprBuilder out(pretty);
		out.text("(");

		auto count = numItems();
		for (SSize i = 0; i < count; ++i) {
			auto elem = at(i);
			if (elem != nullptr)
				out.markup(elem->repr(pretty));

			if (i + 1 < count)
				out.text(", ");
		}

		// A trailing comma is what distinguishes a one-element tuple from a parenthesized value.
		out.text(count == 1 ? ",)" : ")");
		return out.str();
	}

}
