#include "RemotePyUnicodeObject.h"
#include "PyStringValue.h"

#include "utils/lossless_cast.h"
#include "utils/ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
#include <vector>
#include <iterator>
#include <sstream>
#include <iomanip>
using namespace std;

namespace {
	// Python3 strings follow an c-style "inheritance" heirarchy where the base struct is stored
	// as the first member of the derived with a name of `_base`.
	// From most to least derived, this looks like:
	//     PyUnicodeObject -> PyCompactUnicodeObject -> PyASCIIObject.
	//
	// This function returns a field regardless of which `_base` it's held in.
	ExtRemoteTyped getField(ExtRemoteTyped obj, const string& fieldName)
	{
		// Follow the _base until we find a field or there are no more bases.
		while (!obj.HasField(fieldName.c_str()) && obj.HasField("_base")) {
			obj = obj.Field("_base");
		}
		
		return obj.Field(fieldName.c_str());
	}
}


RemotePyUnicodeObject::RemotePyUnicodeObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyASCIIObject")
{
	// We stared off with the base type of PyASCIIObject.
	// Up-cast ourselved to the most derived type possible.
	if (!isAscii()) {
		if (isCompact()) {
			remoteObj().Set("PyCompactUnicodeObject", objectAddress);
		} else {
			remoteObj().Set("PyUnicodeObject", objectAddress);
		}
	}
}


auto RemotePyUnicodeObject::interningState() const -> InterningState
{
	auto internedField = getField(remoteObj(), "state").Field("interned");
	return utils::readIntegral<InterningState>(internedField);
}


auto RemotePyUnicodeObject::kind() const -> Kind
{
	auto kindField = getField(remoteObj(), "state").Field("kind");
	return utils::readIntegral<Kind>(kindField);
}


auto RemotePyUnicodeObject::isCompact() const -> bool
{
	auto compactField = getField(remoteObj(), "state").Field("compact");
	return utils::readIntegral<bool>(compactField);
}


auto RemotePyUnicodeObject::isAscii() const -> bool
{
	auto asciiField = getField(remoteObj(), "state").Field("ascii");
	return utils::readIntegral<bool>(asciiField);
}


auto RemotePyUnicodeObject::isReady() const -> bool
{
	auto readyField = getField(remoteObj(), "state").Field("ready");
	return utils::readIntegral<bool>(readyField);
}


auto RemotePyUnicodeObject::stringLength() const -> SSize
{
	if (!isReady())
		return 0; //< TODO: Consider supporting non-ready strings.

	auto lengthField = getField(remoteObj(), "length");
	return utils::readIntegral<SSize>(lengthField);
}


namespace {
	// For now, these are dumb conversions. They will clip anything outside the bounds of a char.
	// I'm not too concerned with this since the debugger APIs we're calling don't display unicode
	// particularly well anyway.
	string wstring_to_string(wstring str)
	{
		return { begin(str), end(str) }; //< TODO: Convert.
	}

	string utf8_to_string(string str)
	{
		return str; //< TODO: Convert.
	}

	string utf16_to_string(u16string str)
	{
		return { begin(str), end(str) }; //< TODO: Convert.
	}

	string utf32_to_string(u32string str)
	{
		return { begin(str), end(str) }; //< TODO: Convert.
	}
}


auto RemotePyUnicodeObject::stringValue() const -> string
{
	auto wstrField = getField(remoteObj(), "wstr");
	const auto length = utils::lossless_cast<ULONG>(stringLength());
	if (length == 0)
		return { };

	if (wstrField.GetPtr() != 0) {
		auto buffer = utils::readArray<wchar_t>(wstrField, length);

		if (isAscii()) {
			return { begin(buffer), end(buffer) };
		} else {
			return wstring_to_string({ begin(buffer), end(buffer) });
		}
	} else {
		Offset dataPtr = 0;
		if (isCompact()) {
			dataPtr = offset() + remoteObj().Dereference().GetTypeSize();
		} else {
			dataPtr = remoteObj().Field("data").Field("any").GetPtr();
		}
		
		switch (kind()) {
			case Kind::Wchar:
			{
				ExtRemoteTyped data("wchar_t", dataPtr, true);
				auto buffer = utils::readArray<wchar_t>(data, length);
				return { begin(buffer), end(buffer) };
			} break;
			case Kind::OneByte:
			{
				ExtRemoteTyped data("Py_UCS1", dataPtr, true);
				auto buffer = utils::readArray<char>(data, length);
				return utf8_to_string({ begin(buffer), end(buffer) });
			} break;
			case Kind::TwoByte:
			{
				ExtRemoteTyped data("Py_UCS2", dataPtr, true);
				auto buffer = utils::readArray<char16_t>(data, length);
				return utf16_to_string({ begin(buffer), end(buffer) });
			} break;
			case Kind::FourByte:
			{
				ExtRemoteTyped data("Py_UCS4", dataPtr, true);
				auto buffer = utils::readArray<char32_t>(data, length);
				return utf16_to_string({ begin(buffer), end(buffer) });
			} break;
			default:
			{
				throw runtime_error("Unexpected `kind` when decoding string value.");
			} break;
		}
	}
}


auto RemotePyUnicodeObject::repr(bool /*pretty*/) const -> string
{
	if (!isReady())
		return "<str object not ready>";

	ostringstream oss;
	oss << quoted(stringValue()); //< Escape the string.
	return oss.str();
}
