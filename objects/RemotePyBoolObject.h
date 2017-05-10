#pragma once

#include "RemotePyObject.h"
#include <string>
#include <cstdint>

// Represents a RemotePyBoolObject in the debuggee's address space.
class RemotePyBoolObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyBoolObject(Offset objectAddress);

public: // Members.
	bool boolValue() const;
	std::string repr(bool pretty = true) const override;

};