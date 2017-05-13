#include "RemotePyDictObject.h"

#include "objects.h"

#include <engextcpp.hpp>

#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
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


auto RemotePyDictObject::repr(bool pretty) const -> string
{
	const auto elementSeparator = (pretty) ? "\n" : " "; //< Use a newline when pretty-print is on.
	const auto indentation = (pretty) ? "\t" : ""; //< Indent only when pretty is on.

	ostringstream oss;
	oss << '{' << elementSeparator;

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

		oss << indentation << key->repr(pretty) << ": " << value->repr(pretty) << ',' << elementSeparator;
	}

	oss << elementSeparator << '}';
	return oss.str();
}
