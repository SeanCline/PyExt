#include "PyListObject.h"

#include "utils/lossless_cast.h"

#include <engextcpp.hpp>

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
using namespace std;

namespace PyExt::Remote {

	PyListObject::PyListObject(Offset objectAddress)
		: PyVarObject(objectAddress, "PyListObject")
	{
	}


	auto PyListObject::numItems() const -> SSize
	{
		return PyVarObject::size();
	}


	auto PyListObject::at(SSize index) const -> unique_ptr<PyObject>
	{
		if (index < 0 || index >= numItems())
			throw out_of_range("PyListObject::at index out of range.");

		auto obj = remoteType();
		auto itemPtr = obj.Field("ob_item").ArrayElement(index).GetPtr();
		return PyObject::make(itemPtr);
	}


	auto PyListObject::listValue() const -> vector<unique_ptr<PyObject>>
	{
		auto count = utils::lossless_cast<size_t>(numItems());
		vector<unique_ptr<PyObject>> values(count);

		for (size_t i = 0; i < count; ++i) {
			values[i] = at(i);
		}

		return values;
	}


	auto PyListObject::repr(bool pretty) const -> string
	{
		ostringstream oss;
		oss << "[ ";

		auto count = numItems();
		for (SSize i = 0; i < count; ++i) {
			auto elem = at(i);
			if (elem != nullptr)
				oss << elem->repr(pretty);

			if (i + 1 < count)
				oss << ", ";
		}

		oss << " ]";
		return oss.str();
	}

}