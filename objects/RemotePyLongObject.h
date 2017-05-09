#pragma once

#include "RemotePyVarObject.h"
#include <string>
#include <cstdint>

// Represents a PyLongObject in the debuggee's address space.
class RemotePyLongObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	RemotePyLongObject(Offset objectAddress);

public: // Members.
	bool isNegative() const;
	std::string repr(bool pretty = true) const override;

};