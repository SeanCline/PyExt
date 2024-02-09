#pragma once

#include "PyObject.h"
#include <string>
#include <vector>
#include <cstdint>

namespace PyExt::Remote {

	/// Represents a PyFrameObject in the debuggee's address space.
	class PYEXT_PUBLIC PyCodeObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyCodeObject(Offset objectAddress);

	public:
		// Members.
		auto numberOfLocals() const -> int;
		auto firstLineNumber() const -> int;
		auto lineNumberFromInstructionOffset(int instruction) const -> int;
		auto varNames() const->std::vector<std::string>;
		auto freeVars() const->std::vector<std::string>;
		auto cellVars() const->std::vector<std::string>;
		auto filename() const -> std::string;
		auto name() const -> std::string;
		auto lineNumberTableOld() const -> std::vector<std::uint8_t>;
		auto lineNumberTableNew() const -> std::vector<std::uint8_t>;
		auto repr(bool pretty = true) const -> std::string override;

	protected:
		// Helpers.
		auto lineNumberFromInstructionOffsetOld(int instruction, const std::vector<uint8_t> &lnotab) const -> int;
		auto lineNumberFromInstructionOffsetNew(int instruction, const std::vector<uint8_t> &lnotab) const -> int;
		auto readStringTuple(std::string name) const -> std::vector<std::string>;
	};

}