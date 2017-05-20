#pragma once

#include "RemotePyObject.h"
#include <string>
#include <vector>
#include <cstdint>

// Represents a PyFrameObject in the debuggee's address space.
class RemotePyCodeObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyCodeObject(Offset objectAddress);

public: // Members.
	auto firstLineNumber() const -> int;
	auto lineNumberFromInstructionOffset(int instruction) const -> int;
	auto filename() const -> std::string;
	auto name() const -> std::string;
	auto lineNumberTable() const -> std::vector<std::uint8_t>;
	auto repr(bool pretty = true) const -> std::string override;

};