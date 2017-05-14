#include "RemotePyDictObject.h"

#include "objects.h"

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


auto RemotePyDictObject::numUsed() const -> SSize
{
	return remoteObj().Field("ma_used").GetLong64();
}


auto RemotePyDictObject::numMask() const -> SSize
{
	return remoteObj().Field("ma_mask").GetLong64();
}


auto RemotePyDictObject::pairValues() const -> vector<pair<unique_ptr<RemotePyObject>, unique_ptr<RemotePyObject>>>
{
	vector<pair<unique_ptr<RemotePyObject>, unique_ptr<RemotePyObject>>> pairs;

	auto tableSize = numMask();
	auto table = remoteObj().Field("ma_table");
	for (SSize i = 0; i <= tableSize; ++i) {
		auto dictEntry = table.ArrayElement(i);

		auto keyPtr = dictEntry.Field("me_key").GetPtr();
		auto valuePtr = dictEntry.Field("me_value").GetPtr();

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
