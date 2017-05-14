#include "RemotePyCodeObject.h"

#include "RemotePyStringObject.h"
#include "utils/ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
#include <vector>
#include <iterator>
#include <cstdint>
#include <cassert>
using namespace std;


RemotePyCodeObject::RemotePyCodeObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyCodeObject")
{
}


auto RemotePyCodeObject::firstLineNumber() const -> int
{
	auto firstlineno = remoteObj().Field("co_firstlineno");
	return utils::readIntegral<int>(firstlineno);
}


auto RemotePyCodeObject::lineNumberFromInstructionOffset(int instruction) const -> int
{
	// TODO: Consider caching this table in an ordered container.
	auto lnotab = lineNumberTable();

	// This code is explained in the CPython codebase in Objects/lnotab_notes.txt
	int lineno = 0;
	int addr = 0;
	auto last = end(lnotab);
	for (auto it = begin(lnotab); it != last; ++it) {
		auto addr_incr = *it++;

		if (it == last) {
			assert(false && "co_lnotab had an odd number of elements.");
			break; //< For now, just return the line number we've calculated so far.
		}

		auto line_incr = *it;

		addr += addr_incr;
		if (addr > instruction)
			break;

		if (line_incr >= 0x80)
			lineno -= 0x100;

		lineno += line_incr;
	}

	return firstLineNumber() + lineno;
}


auto RemotePyCodeObject::filename() const -> string
{
	auto objPtr = remoteObj().Field("co_filename").GetPtr();
	if (objPtr == 0)
		return {};
	
	auto filename = RemotePyStringObject(objPtr);
	return filename.stringValue();
}


auto RemotePyCodeObject::name() const -> string
{
	auto objPtr = remoteObj().Field("co_name").GetPtr();
	if (objPtr == 0)
		return {};
	
	auto nameObject = RemotePyStringObject(objPtr);
	return nameObject.stringValue();
}


auto RemotePyCodeObject::lineNumberTable() const -> vector<uint8_t>
{
	auto objPtr = remoteObj().Field("co_lnotab").GetPtr();
	if (objPtr == 0)
		return {};

	auto tableString = RemotePyStringObject(objPtr).stringValue();
	return vector<uint8_t>(begin(tableString), end(tableString));
}


auto RemotePyCodeObject::repr(bool /*pretty*/) const -> string
{
	return "<code object, file \"" + filename() + "\", line " + to_string(firstLineNumber()) + ">";
}
