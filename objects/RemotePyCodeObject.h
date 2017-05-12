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
	int firstLineNumber() const;
	int lineNumberFromInstructionOffset(int instruction) const;
	std::string filename() const;
	std::string name() const;
	std::vector<std::uint8_t> lineNumberTable() const;
	
	std::string repr(bool pretty = true) const override;

};