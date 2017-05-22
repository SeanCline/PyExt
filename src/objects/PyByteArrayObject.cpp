#include "PyByteArrayObject.h"
#include "PyStringValue.h"

#include "utils/lossless_cast.h"

#include <engextcpp.hpp>

#include <string>
#include <vector>
#include <iterator>
using namespace std;


namespace PyExt::Remote {

	PyByteArrayObject::PyByteArrayObject(Offset objectAddress)
		: PyVarObject(objectAddress, "PyByteArrayObject")
	{
	}


	auto PyByteArrayObject::stringLength() const -> SSize
	{
		auto len = size();
		return (len < 0) ? 0 : len;
	}


	auto PyByteArrayObject::stringValue() const -> string
	{
		auto byteVec = arrayValue();
		return { begin(byteVec), end(byteVec) };
	}


	auto PyByteArrayObject::arrayValue() const -> std::vector<uint8_t>
	{
		auto len = stringLength();
		if (len <= 0)
			return { };

		vector<uint8_t> buff(utils::lossless_cast<size_t>(len), '\0');
		auto bytesField = remoteType().Field("ob_bytes");
		bytesField.Dereference().ReadBuffer(buff.data(), static_cast<ULONG>(buff.size()));
		return buff;
	}


	auto PyByteArrayObject::repr(bool /*pretty*/) const -> string
	{
		return "bytearray(b"s + escapeAndQuoteString(stringValue()) + ")"s;
	}

}
