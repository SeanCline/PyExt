#pragma once

#include "RemotePyObject.h"
#include <string>

// Represents an object of type NoneType. The most boring of the Python types.
class RemotePyNoneObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyNoneObject(Offset objectAddress);

public: // Members.
	auto repr(bool pretty = true) const -> std::string override;

};