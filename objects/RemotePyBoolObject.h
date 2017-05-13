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
	auto boolValue() const -> bool;
	auto repr(bool pretty = true) const -> std::string override;

};