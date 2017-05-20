#pragma once

#include "RemotePyVarObject.h"
#include <string>
#include <cstdint>

// Represents a PyLongObject in the debuggee's address space.
class RemotePyLongObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyLongObject(Offset objectAddress);

public: // Members.
	auto isNegative() const -> bool;
	auto repr(bool pretty = true) const -> std::string override;

};