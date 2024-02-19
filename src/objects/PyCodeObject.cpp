#include "PyCodeObject.h"

#include "PyStringValue.h"
#include "PyTupleObject.h"

#include "utils/lossless_cast.h"
#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
#include <vector>
#include <iterator>
#include <cstdint>
#include <cassert>
using namespace std;


namespace PyExt::Remote {

	PyCodeObject::PyCodeObject(Offset objectAddress)
		: PyObject(objectAddress, "PyCodeObject")
	{
	}


	auto PyCodeObject::numberOfLocals() const -> int
	{
		auto nlocals = remoteType().Field("co_nlocals");
		return utils::readIntegral<int>(nlocals);
	}


	auto PyCodeObject::firstLineNumber() const -> int
	{
		auto firstlineno = remoteType().Field("co_firstlineno");
		return utils::readIntegral<int>(firstlineno);
	}


	auto PyCodeObject::lineNumberFromInstructionOffset(int instruction) const -> int
	{
		// TODO: Consider caching this table in an ordered container.
		auto lnotab = lineNumberTableNew();
		if (lnotab.empty()) {
			// Python 3.9 and below
			lnotab = lineNumberTableOld();
			if (!lnotab.empty()) {
				return lineNumberFromInstructionOffsetOld(instruction, lnotab);
			}
		}
		else {
			// Python 3.10
			// We have to multiply instruction with the size of a code unit
			// https://github.com/python/cpython/blob/v3.10.0/Python/ceval.c#L5485
			return lineNumberFromInstructionOffsetNew(instruction * 2, lnotab);
		}

		return firstLineNumber();
	}



	auto PyCodeObject::lineNumberFromInstructionOffsetOld(int instruction, const vector<uint8_t> &lnotab) const -> int
	{
		// Python 3.9 and below
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


	auto PyCodeObject::lineNumberFromInstructionOffsetNew(int instruction, const vector<uint8_t>& linetable) const -> int
	{
		// Python 3.10
		// This code is based on co_lines(), which can be found in the CPython codebase in Objects/lnotab_notes.txt
		int line = firstLineNumber();
		int addr = 0;
		auto last = end(linetable);
		for (auto it = begin(linetable); it != last; ++it) {
			auto sdelta = *it++;

			if (it == last) {
				assert(false && "co_linetable had an odd number of elements.");
				break; //< For now, just return the line number we've calculated so far.
			}

			auto ldelta = static_cast<int8_t>(*it);

			addr += sdelta;
			if (ldelta == -128)  // no line number, treated as delta of zero
				ldelta = 0;
			line += ldelta;
			if (addr > instruction)
				break;
		}

		return line;
	}


	auto PyCodeObject::lineNumberFromPrevInstruction(int instruction) const -> int
	{
		// Python 3.11 and above, see Objects/locations.md
		auto codeAdaptivePtr = remoteType().Field("co_code_adaptive").GetPointerTo();
		auto firstInstruction = utils::readIntegral<int>(codeAdaptivePtr);
		instruction -= firstInstruction;

		int line = firstLineNumber();
		int addr = 0;
		auto linetable = lineNumberTableNew();
		auto last = end(linetable);
		for (auto it = begin(linetable); it != last;) {
			auto byte = *it++;
			auto length = (byte & 7) + 1;
			addr += length * 2;
			auto code = (byte >> 3) & 15;

			if (code <= 9) {
				// short form: 2 bytes, no line delta
				it++;
			} else if (code >= 10 && code <= 12) {
				// one line form: 3 bytes, line delta = code - 10
				it += 2;
				line += code - 10;
			} else if (code == 13) {
				// no column info
				line += readSvarint(it);  // start line
			} else if (code == 14) {
				// long form
				line += readSvarint(it);  // start line
				readVarint(it);  // end line
				readVarint(it);  // start column
				readVarint(it);  // end column
			} else if (code == 15) {
				// no location
			} else {
				assert(false && "unexpected code in co_linetable.");
				break; //< For now, just return the line number we've calculated so far.
			}

			if (addr > instruction)
				break;
		}

		return line;
	}


	auto PyCodeObject::varNames() const -> vector<string>
	{
		return readStringTuple("co_varnames");
	}


	auto PyCodeObject::freeVars() const -> vector<string>
	{
		return readStringTuple("co_freevars");
	}


	auto PyCodeObject::cellVars() const -> vector<string>
	{
		return readStringTuple("co_cellvars");
	}


	auto PyCodeObject::localsplusNames() const -> vector<string>
	{
		return readStringTuple("co_localsplusnames");
	}


	auto PyCodeObject::filename() const -> string
	{
		auto filenameStr = utils::fieldAsPyObject<PyStringValue>(remoteType(), "co_filename");
		if (filenameStr == nullptr)
			return { };

		return filenameStr->stringValue();
	}


	auto PyCodeObject::name() const -> string
	{
		auto nameStr = utils::fieldAsPyObject<PyStringValue>(remoteType(), "co_name");
		if (nameStr == nullptr)
			return { };

		return nameStr->stringValue();
	}


	auto PyCodeObject::lineNumberTableOld() const -> vector<uint8_t>
	{
		auto codeStr = utils::fieldAsPyObject<PyStringValue>(remoteType(), "co_lnotab");
		if (codeStr == nullptr)
			return { };

		auto tableString = codeStr->stringValue();
		return vector<uint8_t>(begin(tableString), end(tableString));
	}

	auto PyCodeObject::lineNumberTableNew() const -> vector<uint8_t>
	{
		auto codeStr = utils::fieldAsPyObject<PyStringValue>(remoteType(), "co_linetable");
		if (codeStr == nullptr)
			return { };

		auto tableString = codeStr->stringValue();
		return vector<uint8_t>(begin(tableString), end(tableString));
	}


	auto PyCodeObject::repr(bool pretty) const -> string
	{
		string repr =  "<code object, file \"" + filename() + "\", line " + to_string(firstLineNumber()) + ">";
		if (pretty)
			return utils::link(repr, "!pyobj 0n"s + to_string(offset()));
		return repr;
	}


	auto PyCodeObject::readStringTuple(string name) const -> vector<string>
	{
		auto tuplePtr = utils::fieldAsPyObject<PyTupleObject>(remoteType(), name);
		if (tuplePtr == nullptr)
			return { };

		auto count = utils::lossless_cast<size_t>(tuplePtr->numItems());
		vector<string> values(count);

		for (size_t i = 0; i < count; ++i) {
			auto pyVarName = tuplePtr->at(i);
			values[i] = pyVarName->repr();
		}

		return values;
	}


	auto PyCodeObject::readVarint(std::vector<uint8_t>::const_iterator& it) const -> unsigned int
	{
		auto ret = 0;
		uint8_t byte;
		auto shift = 0;
		do {
			byte = *it++;
			ret += (byte & 63) << shift;
			shift += 6;
		} while ((byte & 64) != 0);
		return ret;
	}


	auto PyCodeObject::readSvarint(std::vector<uint8_t>::const_iterator& it) const -> int
	{
		auto varint = readVarint(it);
		auto svarint = static_cast<int>(varint >> 1);
		if ((varint & 1) != 0)
			svarint = -svarint;
		return svarint;
	}
}
