#include "RemotePyDictObject.h"

#include "objects.h"
#include "utils/ExtHelpers.h"

#include <engextcpp.hpp>

#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <utility>
using namespace std;

RemotePyDictObject::RemotePyDictObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyDictObject")
{
}


namespace {

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
		auto size = utils::readIntegral<RemotePyObject::SSize>(sizeField);
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

		auto entriesPtr = keys.Field("dk_indices").Field("as_1").GetPtr() + (size * indexSize);
		return ExtRemoteTyped("PyDictKeyEntry", entriesPtr, true);
	}


	auto getEntriesTableSize(ExtRemoteTyped dictObj) -> RemotePyObject::SSize
	{
		// Python 2.
		if (dictObj.HasField("ma_mask")) {
			// Mask is table size (a power of 2) minus 1.
			auto mask = dictObj.Field("ma_mask");
			return utils::readIntegral<RemotePyObject::SSize>(mask) + 1;
		}


		// Python 3.5
		if (!dictObj.Field("ma_keys").HasField("dk_nentries")) {
			auto sizeField = dictObj.Field("ma_keys").Field("dk_size");
			return utils::readIntegral<RemotePyObject::SSize>(sizeField);
		}

		// Python 3.6
		auto numEntriesField = dictObj.Field("ma_keys").Field("dk_nentries");
		return utils::readIntegral<RemotePyObject::SSize>(numEntriesField);
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


auto RemotePyDictObject::pairValues() const -> vector<pair<unique_ptr<RemotePyObject>, unique_ptr<RemotePyObject>>>
{
	vector<pair<unique_ptr<RemotePyObject>, unique_ptr<RemotePyObject>>> pairs;

	auto table = getEntriesTable(remoteObj());
	const auto tableSize = getEntriesTableSize(remoteObj());
	const bool isCombined = getIsCombined(remoteObj());
	for (SSize i = 0; i < tableSize; ++i) {
		auto dictEntry = table.ArrayElement(i);

		Offset keyPtr = dictEntry.Field("me_key").GetPtr();
		Offset valuePtr = 0;
		if (isCombined) {
			valuePtr = dictEntry.Field("me_value").GetPtr();
		} else {
			valuePtr = remoteObj().Field("ma_values").ArrayElement(i).GetPtr();
		}

		if (keyPtr == 0 || valuePtr == 0) //< The hash bucket might be empty.
			continue;

		auto key = makeRemotePyObject(keyPtr);
		auto value = makeRemotePyObject(valuePtr);
		pairs.push_back(make_pair(move(key), move(value)));
	}

	return pairs;
}


auto RemotePyDictObject::repr(bool pretty) const -> string
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

	oss << elementSeparator << '}';
	return oss.str();
}
