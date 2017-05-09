#pragma once

#include "RemotePyVarObject.h"
#include <string>

// Represents an object of type NoneType. The most boring of the Python types.
class RemotePyNoneObject : public RemotePyObject
{

public: // Construction/Destruction.
	RemotePyNoneObject(Offset objectAddress);

public: // Members.
	std::string repr(bool pretty = true) const override;

};