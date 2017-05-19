#include "RemotePyByteArrayObject.h"
#include "PyStringValue.h"

#include "utils/lossless_cast.h"

#include <engextcpp.hpp>

#include <string>
#include <vector>
#include <iterator>
using namespace std;


RemotePyByteArrayObject::RemotePyByteArrayObject(Offset objectAddress)
	: RemotePyVarObject(objectAddress, "PyByteArrayObject")
{
}


auto RemotePyByteArrayObject::stringLength() const -> SSize
{
	auto len = size();
	return (len < 0) ? 0 : len;
}


auto RemotePyByteArrayObject::stringValue() const -> string
{
	auto byteVec = arrayValue();
	return { begin(byteVec), end(byteVec) };
}


auto RemotePyByteArrayObject::arrayValue() const -> std::vector<uint8_t>
{
	auto len = stringLength();
	if (len <= 0)
		return { };

	vector<uint8_t> buff(utils::lossless_cast<size_t>(len), '\0');
	auto bytesField = remoteObj().Field("ob_bytes");
	bytesField.Dereference().ReadBuffer(buff.data(), static_cast<ULONG>(buff.size()));
	return buff;
}


auto RemotePyByteArrayObject::repr(bool /*pretty*/) const -> string
{
	return "bytearray(b"s + escapeAndQuoteString(stringValue()) + ")"s;
}
