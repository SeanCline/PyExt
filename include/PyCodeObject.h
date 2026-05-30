#pragma once

#include "PyObject.h"
#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace PyExt::Remote {

	/// Represents a PyCodeObject in the debuggee's address space.
	class PYEXT_PUBLIC PyCodeObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyCodeObject(Offset objectAddress);

	public:
		// Members.
		auto numberOfLocals() const -> int;
		auto firstLineNumber() const -> int;
		auto lineNumberFromInstructionOffset(int instruction) const -> int;

		// Converts the address of an instruction to its line number.
		// Under Py_GIL_DISABLED the bytecode lives in a per-thread copy
		// (co_tlbc->entries[tlbcIndex]); pass the owning thread's index when
		// known. nullopt (the default) falls back to entries[0] — the main
		// thread's inline copy, equivalent to co_code_adaptive on a GIL build.
		auto lineNumberFromPrevInstruction(Offset instructionAddress, std::optional<int> tlbcIndex = std::nullopt) const -> int;

		auto varNames() const -> std::vector<std::string>;
		auto freeVars() const -> std::vector<std::string>;
		auto cellVars() const -> std::vector<std::string>;
		auto localsplusNames() const -> std::vector<std::string>;
		auto filename() const -> std::string;
		auto name() const -> std::string;
		auto lineNumberTableOld() const -> std::vector<std::uint8_t>;
		auto lineNumberTableNew() const -> std::vector<std::uint8_t>;

		// Index (in code units) of the first "complete" instruction. Returns nullopt on versions (<3.12) where `_co_firsttraceable` is not present .
		auto firstTraceableIndex() const -> std::optional<int>;

		// Address of the first byte of the executing bytecode. On the GIL build
		// this is co_code_adaptive on the code object itself; under Py_GIL_DISABLED
		// it is co_tlbc->entries[tlbcIndex], a per-thread copy. nullopt as
		// tlbcIndex falls back to entries[0] — correct for the main thread,
		// approximate for others. Returns nullopt on Python versions that expose
		// neither field.
		auto bytecodeStartAddress(std::optional<int> tlbcIndex = std::nullopt) const -> std::optional<Offset>;

		auto repr(bool pretty = true) const -> std::string override;

	protected:
		// Helpers.
		auto lineNumberFromInstructionOffsetOld(int instruction, const std::vector<uint8_t> &lnotab) const -> int;
		auto lineNumberFromInstructionOffsetNew(int instruction, const std::vector<uint8_t> &linetable) const -> int;
		auto readStringTuple(std::string name) const -> std::vector<std::string>;
		auto readVarint(std::vector<uint8_t>::const_iterator &index) const -> unsigned int;
		auto readSvarint(std::vector<uint8_t>::const_iterator &index) const -> int;
	};

}