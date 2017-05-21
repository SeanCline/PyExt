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

	public: // Members.
		auto firstLineNumber() const -> int;
		auto lineNumberFromInstructionOffset(int instruction) const -> int;
		auto filename() const -> std::string;
		auto name() const -> std::string;
		auto lineNumberTable() const -> std::vector<std::uint8_t>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}