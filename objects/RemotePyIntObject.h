#pragma once

#include "RemotePyObject.h"
#include <string>
#include <cstdint>

// Represents a PyIntObject in the debuggee's address space.
class RemotePyIntObject : public RemotePyObject
{

public: // Construction/Destruction.
	RemotePyIntObject(Offset objectAddress);

public: // Members.
	int32_t intValue() const;
	std::string repr(bool pretty = true) const override;

};