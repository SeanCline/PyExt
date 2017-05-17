#include "RemotePyStringObject.h"

#include "utils/lossless_cast.h"

#include <engextcpp.hpp>

#include <string>
#include <vector>
#include <iterator>
using namespace std;


RemotePyBaseStringObject::RemotePyBaseStringObject(Offset objectAddress, const std::string& typeName)
	: RemotePyVarObject(objectAddress, typeName)
{
}


auto RemotePyBaseStringObject::stringLength() const -> SSize
{
	auto len = size();
	return (len < 0) ? 0 : len; //< Don't let the length go negative. Python enforces this.
}


auto RemotePyBaseStringObject::stringValue() const -> string
{
	auto len = stringLength();
	if (len <= 0)
		return { };

	vector<char> buff(utils::lossless_cast<size_t>(len), '\0');

	// String objects in Python can embed \0's, especially in Python2, when used to store bytes.
	// GetString() stops at the first \0 so ReadBuffer() is a better choice.
	auto sval = remoteObj().Field("ob_sval");
	sval.Dereference().ReadBuffer(buff.data(), static_cast<ULONG>(buff.size()));
	return { begin(buff), end(buff) };
}


auto RemotePyBaseStringObject::repr(bool /*pretty*/) const -> string
{
	return "b" + escapeAndQuoteString(stringValue());
}



RemotePyBytesObject::RemotePyBytesObject(Offset objectAddress)
	: RemotePyBaseStringObject(objectAddress, "PyBytesObject")
{
}


RemotePyStringObject::RemotePyStringObject(Offset objectAddress)
	: RemotePyBaseStringObject(objectAddress, "PyStringObject")
{
}
