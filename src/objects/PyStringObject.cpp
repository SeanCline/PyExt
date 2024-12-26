#include "PyStringObject.h"

#include "utils/lossless_cast.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
#include <stdexcept>
using namespace std;

namespace PyExt::Remote {

	PyBaseStringObject::PyBaseStringObject(Offset objectAddress, const std::string& typeName)
		: PyVarObject(objectAddress, typeName)
	{
	}


	auto PyBaseStringObject::stringLength() const -> SSize
	{
		auto len = size();

		// Don't let the length go negative. Python enforces this invariant.
		if (len < 0)
			throw runtime_error("Negative size in PyBaseStringObject."); 

		return len; 
	}


	auto PyBaseStringObject::stringValue() const -> string
	{
		// String and Bytes objects in Python can embed \0's so we read `len` bytes from the debuggee
		// instead of stopping at the first \0.
		const auto len = stringLength();
		if (len == 0)
			return ""s;

		string buffer(utils::lossless_cast<size_t>(len), '\0');
		auto sval = remoteType().Field("ob_sval");
		sval.Dereference().ReadBuffer(buffer.data(), utils::lossless_cast<ULONG>(buffer.size()));
		return buffer;
	}


	auto PyBaseStringObject::repr(bool pretty) const -> string
	{
		string repr = escapeAndQuoteString(stringValue());
		return pretty ? utils::escapeDml(repr) : repr;
	}



	PyBytesObject::PyBytesObject(Offset objectAddress)
		: PyBaseStringObject(objectAddress, "PyBytesObject")
	{
	}


	auto PyBytesObject::repr(bool pretty) const -> string
	{
		return "b" + PyBaseStringObject::repr(pretty);
	}


	PyStringObject::PyStringObject(Offset objectAddress)
		: PyBaseStringObject(objectAddress, "PyStringObject")
	{
	}

}
