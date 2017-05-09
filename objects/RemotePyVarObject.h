#pragma once

#include "RemotePyObject.h"
#include <cstdint>

// Represents a PyVarObject (objects with a dynamic allocation) in the debuggee's address space.
// Base class for all variable-sized objects.
class RemotePyVarObject : public RemotePyObject
{

public: // Construction/Destruction.
	RemotePyVarObject(Offset objectAddress, const std::string& typeName = "PyVarObject");

public: // Members.
	SSize getSize() const;

};