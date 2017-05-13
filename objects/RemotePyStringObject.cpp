#include "RemotePyStringObject.h"

#include <engextcpp.hpp>
#include <string>
#include <vector>
#include <iterator>
#include <sstream>
#include <iomanip>
using namespace std;

RemotePyStringObject::RemotePyStringObject(Offset objectAddress)
	: RemotePyVarObject(objectAddress, "PyStringObject")
{
}


auto RemotePyStringObject::stringLength() const -> SSize
{
	auto len = size();
	return (len < 0) ? 0 : len; //< Don't let the length go negative. Python enforces this.
}


auto RemotePyStringObject::stringValue() const -> string
{
	auto len = stringLength();
	if (len <= 0)
		return { };

	vector<char> buff(len, '\0');

	// String objects in Python can embed \0's, especially in Python2, when used to store bytes.
	// GetString() stops at the first \0 so ReadBuffer() is a better choice.
	auto sval = remoteObj().Field("ob_sval");
	sval.Dereference().ReadBuffer(buff.data(), static_cast<ULONG>(buff.size()));
	return { begin(buff), end(buff) };
}


auto RemotePyStringObject::repr(bool /*pretty*/) const -> string
{
	ostringstream oss;
	oss << quoted(stringValue()); //< Escape the string.
	return oss.str();
}
