#pragma once

#include "RemotePyObject.h"
#include <string>
#include <cstdint>

// Represents a PyIntObject in the debuggee's address space.
class RemotePyIntObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyIntObject(Offset objectAddress);

public: // Members.
	auto intValue() const -> std::int32_t;
	auto repr(bool pretty = true) const -> std::string override;

};