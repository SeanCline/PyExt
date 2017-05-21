#include "PyStringObject.h"

#include "utils/lossless_cast.h"

#include <engextcpp.hpp>

#include <string>
#include <vector>
#include <iterator>
using namespace std;

namespace PyExt::Remote {

	PyBaseStringObject::PyBaseStringObject(Offset objectAddress, const std::string& typeName)
		: PyVarObject(objectAddress, typeName)
	{
	}


	auto PyBaseStringObject::stringLength() const -> SSize
	{
		auto len = size();
		return (len < 0) ? 0 : len; //< Don't let the length go negative. Python enforces this.
	}


	auto PyBaseStringObject::stringValue() const -> string
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


	auto PyBaseStringObject::repr(bool /*pretty*/) const -> string
	{
		return "b" + escapeAndQuoteString(stringValue());
	}



	PyBytesObject::PyBytesObject(Offset objectAddress)
		: PyBaseStringObject(objectAddress, "PyBytesObject")
	{
	}


	PyStringObject::PyStringObject(Offset objectAddress)
		: PyBaseStringObject(objectAddress, "PyStringObject")
	{
	}

}
