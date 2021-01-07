#include "PyDictObject.h"

#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <utility>
using namespace std;


namespace {
	using PyExt::Remote::PyObject;

	auto getEntriesTable(ExtRemoteTyped dictObj) -> ExtRemoteTyped
	{
		// Python 2 stores a pointer to the entries table right in `ma_table`.
		if (dictObj.HasField("ma_table"))
			return dictObj.Field("ma_table");

		// Python <= 3.5 stores a pointer to the entries table in `ma_keys->dk_entries`.
		auto keys = dictObj.Field("ma_keys");
		if (keys.HasField("dk_entries"))
			return keys.Field("dk_entries");

		// Python 3.6 uses a "compact" layout where the entries appear after the `ma_keys->dk_indices` table.
		auto sizeField = keys.Field("dk_size");
		auto size = utils::readIntegral<PyObject::SSize>(sizeField);
		auto pointerSize = static_cast<int>(keys.GetPointerTo().GetTypeSize());

		int indexSize = 0;
		if (size <= 0xff) {
			indexSize = 1;
		} else if (size <= 0xffff) {
			indexSize = 2;
		} else if (size <= 0xffffffff) {
			indexSize = 4;
		} else {
			indexSize = pointerSize;
		}

		auto indicies = keys.Field("dk_indices"); // 3.6 and 3.7 both have an indicies field.
		ExtRemoteTyped indiciesPtr;
		if (indicies.HasField("as_1")) {
			// Python 3.6 accesses dk_indicies though a union.
			indiciesPtr = indicies.Field("as_1").GetPointerTo();
		} else {
			// Python 3.7 accesses it as a char[].
			indiciesPtr = indicies.GetPointerTo();
		}

		auto entriesPtr = indiciesPtr.GetPtr() + (size * indexSize);
		return ExtRemoteTyped("PyDictKeyEntry", entriesPtr, true);
	}


	auto getEntriesTableSize(ExtRemoteTyped dictObj) -> PyObject::SSize
	{
		// Python 2.
		if (dictObj.HasField("ma_mask")) {
			// Mask is table size (a power of 2) minus 1.
			auto mask = dictObj.Field("ma_mask");
			return utils::readIntegral<PyObject::SSize>(mask) + 1;
		}


		// Python 3.5
		if (!dictObj.Field("ma_keys").HasField("dk_nentries")) {
			auto sizeField = dictObj.Field("ma_keys").Field("dk_size");
			return utils::readIntegral<PyObject::SSize>(sizeField);
		}

		// Python 3.6
		auto numEntriesField = dictObj.Field("ma_keys").Field("dk_nentries");
		return utils::readIntegral<PyObject::SSize>(numEntriesField);
	}


	auto getIsCombined(ExtRemoteTyped dictObj) -> bool
	{
		// Python 2 tables always act like Python 3 "combined" tables.
		if (dictObj.HasField("ma_mask")) {
			return true;
		}

		// Python 3.
		auto valuesPtr = dictObj.Field("ma_values").GetPtr();
		return (valuesPtr == 0);
	}

}

namespace PyExt::Remote {

	PyDictObject::PyDictObject(Offset objectAddress)
		: PyObject(objectAddress, "PyDictObject")
	{
	}


	auto PyDictObject::pairValues() const -> vector<pair<unique_ptr<PyObject>, unique_ptr<PyObject>>>
	{
		vector<pair<unique_ptr<PyObject>, unique_ptr<PyObject>>> pairs;

		auto table = getEntriesTable(remoteType());
		const auto tableSize = getEntriesTableSize(remoteType());
		const bool isCombined = getIsCombined(remoteType());
		for (SSize i = 0; i < tableSize; ++i) {
			auto dictEntry = table.ArrayElement(i);

			Offset keyPtr = dictEntry.Field("me_key").GetPtr();
			Offset valuePtr = 0;
			if (isCombined) {
				valuePtr = dictEntry.Field("me_value").GetPtr();
			} else {
				valuePtr = remoteType().Field("ma_values").ArrayElement(i).GetPtr();
			}

			if (keyPtr == 0 || valuePtr == 0) //< The hash bucket might be empty.
				continue;

			auto key = PyObject::make(keyPtr);
			auto value = PyObject::make(valuePtr);
			pairs.push_back(make_pair(move(key), move(value)));
		}

		return pairs;
	}


	auto PyDictObject::repr(bool pretty) const -> string
	{
		const auto elementSeparator = (pretty) ? "\n" : " "; //< Use a newline when pretty-print is on.
		const auto indentation = (pretty) ? "\t" : ""; //< Indent only when pretty is on.

		ostringstream oss;
		oss << '{' << elementSeparator;

		for (auto& pairValue : pairValues()) { //< TODO: Structured bindings. for (auto&& [key, value] : pairValues) {
			auto& key = pairValue.first;
			auto& value = pairValue.second;
			oss << indentation << key->repr(pretty) << ": " << value->repr(pretty) << ',' << elementSeparator;
		}

		oss << '}';
		return oss.str();
	}

}
