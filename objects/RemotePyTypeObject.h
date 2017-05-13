#pragma once

#include "RemotePyVarObject.h"
#include <string>

// Represents a PyTypeObject in the debuggee's address space.
class RemotePyTypeObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyTypeObject(Offset objectAddress);

public: // Members.
	std::string name() const;
	std::string documentation() const;
	std::string repr(bool pretty = true) const override;

};