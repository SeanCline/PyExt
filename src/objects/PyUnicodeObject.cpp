#include "PyUnicodeObject.h"
#include "PyStringValue.h"

#include "utils/lossless_cast.h"
#include "../ExtHelpers.h"
#include "../ReprBuilder.h"

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
	auto getField(ExtRemoteTyped obj, const string& fieldName) -> ExtRemoteTyped
	{
		// Follow the _base until we find a field or there are no more bases.
		while (!obj.HasField(fieldName.c_str()) && obj.HasField("_base")) {
			obj = obj.Field("_base");
		}
		
		return obj.Field(fieldName.c_str());
	}

	auto hasField(ExtRemoteTyped obj, const string& fieldName) -> bool
	{
		// Follow the _base until we find a field or there are no more bases.
		while (!obj.HasField(fieldName.c_str()) && obj.HasField("_base")) {
			obj = obj.Field("_base");
		}

		return obj.HasField(fieldName.c_str());
	}
}

namespace PyExt::Remote {

	PyUnicodeObject::PyUnicodeObject(Offset objectAddress)
		: PyObject(objectAddress, "PyASCIIObject")
	{
		// We start off with the base type of PyASCIIObject.
		// Up-cast ourselved to the most derived type possible.
		if (isCompact()) {
			if (isAscii()) {
				remoteType().Set("PyASCIIObject", objectAddress, true);
			} else {
				remoteType().Set("PyCompactUnicodeObject", objectAddress, true);
			}
		} else {
			remoteType().Set("PyUnicodeObject", objectAddress, true);
		}
	}


	auto PyUnicodeObject::interningState() const -> InterningState
	{
		auto internedField = getField(remoteType(), "state").Field("interned");
		return utils::readIntegral<InterningState>(internedField);
	}


	auto PyUnicodeObject::kind() const -> Kind
	{
		auto kindField = getField(remoteType(), "state").Field("kind");
		return utils::readIntegral<Kind>(kindField);
	}


	auto PyUnicodeObject::isCompact() const -> bool
	{
		auto compactField = getField(remoteType(), "state").Field("compact");
		return utils::readIntegral<bool>(compactField);
	}


	auto PyUnicodeObject::isAscii() const -> bool
	{
		auto asciiField = getField(remoteType(), "state").Field("ascii");
		return utils::readIntegral<bool>(asciiField);
	}


	auto PyUnicodeObject::isReady() const -> bool
	{
		if (!hasField(remoteType(), "state"))
			return true;

		auto stateField = getField(remoteType(), "state");
		if (!stateField.HasField("ready"))
			return true; //< ready bit removed from state in Python 3.12+

		auto readyField = stateField.Field("ready");
		return utils::readIntegral<bool>(readyField);
	}


	auto PyUnicodeObject::stringLength() const -> SSize
	{
		if (!isReady())
			return 0; //< TODO: Consider supporting non-ready strings.

		auto lengthField = getField(remoteType(), "length");
		return utils::readIntegral<SSize>(lengthField);
	}


	namespace {

		// Converts a buffer of fixed-width code units to UTF-8.
		template <typename CodeUnit>
		auto codeUnitsToUtf8(const vector<CodeUnit>& codeUnits) -> string
		{
			string utf8;
			utf8.reserve(codeUnits.size());
			for (auto codeUnit : codeUnits)
				utils::appendUtf8(utf8, static_cast<char32_t>(codeUnit));
			return utf8;
		}

	}


	auto PyUnicodeObject::stringValue() const -> string
	{
		const auto length = utils::lossless_cast<ULONG>(stringLength());
		if (length == 0)
			return { };

		const auto stringKind = kind();

		// Legacy strings (Python <=3.11) keep their text in `wstr` until they're made ready.
		// For every other kind `wstr` is only a cache, and its length can differ from `length`
		// when the text has non-BMP characters, so the canonical data below is preferred.
		if (stringKind == Kind::Wchar) {
			if (!hasField(remoteType(), "wstr"))
				throw runtime_error("Unexpected `kind` when decoding string value.");

			auto wstrField = getField(remoteType(), "wstr");
			if (wstrField.GetPtr() == 0)
				return { };
			auto wstr = utils::readArray<wchar_t>(wstrField, length);
			return utils::wideToUtf8({ wstr.data(), wstr.size() });
		}

		Offset dataPtr = 0;
		if (isCompact()) {
			dataPtr = offset() + remoteType().Dereference().GetTypeSize();
		} else {
			dataPtr = remoteType().Field("data").Field("any").GetPtr();
		}

		switch (stringKind) {
		case Kind::OneByte:
		{
			ExtRemoteTyped data("Py_UCS1", dataPtr, true);
			return codeUnitsToUtf8(utils::readArray<uint8_t>(data, length)); //< Latin-1, not UTF-8.
		}
		case Kind::TwoByte:
		{
			ExtRemoteTyped data("Py_UCS2", dataPtr, true);
			return codeUnitsToUtf8(utils::readArray<char16_t>(data, length));
		}
		case Kind::FourByte:
		{
			ExtRemoteTyped data("Py_UCS4", dataPtr, true);
			return codeUnitsToUtf8(utils::readArray<char32_t>(data, length));
		}
		default:
			throw runtime_error("Unexpected `kind` when decoding string value.");
		}
	}


	auto PyUnicodeObject::repr(bool pretty) const -> string
	{
		string repr = isReady() ? escapeAndQuoteString(stringValue(), QuoteType::Single, /*escapeNonAscii=*/ false) : "<str object not ready>";
		return utils::ReprBuilder(pretty).text(repr).str();
	}

}
