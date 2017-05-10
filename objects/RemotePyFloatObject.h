#pragma once

#include "RemotePyObject.h"
#include <string>
#include <cstdint>

// Represents a PyFloatObject in the debuggee's address space.
class RemotePyFloatObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyFloatObject(Offset objectAddress);

public: // Members.
	double floatValue() const;
	std::string repr(bool pretty = true) const override;

};