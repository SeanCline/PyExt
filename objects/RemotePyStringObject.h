#pragma once

#include "RemotePyVarObject.h"
#include <string>

// Represents a PyStringObject in the debuggee's address space.
class RemotePyStringObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyStringObject(Offset objectAddress);

public: // Members.
	std::string stringValue() const;
	std::string repr(bool pretty = true) const override;

};