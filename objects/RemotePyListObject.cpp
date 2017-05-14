#include "RemotePyListObject.h"

#include "objects.h"

#include <engextcpp.hpp>

#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
using namespace std;

RemotePyListObject::RemotePyListObject(Offset objectAddress)
	: RemotePyVarObject(objectAddress, "PyListObject")
{
}


auto RemotePyListObject::numItems() const -> SSize
{
	return RemotePyVarObject::size();
}


auto RemotePyListObject::at(SSize index) const -> unique_ptr<RemotePyObject>
{
	if (index < 0 || index >= numItems())
		throw out_of_range("RemotePyListObject::at index out of range.");

	auto obj = remoteObj();
	auto itemPtr = obj.Field("ob_item").ArrayElement(index).GetPtr();
	return makeRemotePyObject(itemPtr);
}


auto RemotePyListObject::listValue() const -> vector<unique_ptr<RemotePyObject>>
{
	auto count = numItems();
	vector<unique_ptr<RemotePyObject>> values(count);
	
	for (SSize i = 0; i < count; ++i) {
		values[i] = at(i);
	}

	return values;
}


auto RemotePyListObject::repr(bool /*pretty*/) const -> string
{
	ostringstream oss;
	oss << "[ ";

	auto count = numItems();
	for (SSize i = 0; i < count; ++i) {
		auto elem = at(i);
		if (elem != nullptr)
			oss << elem->repr();

		if (i+1 < count)
			oss << ", ";
	}

	oss << " ]";
	return oss.str();
}
