#pragma once

#include "RemotePyVarObject.h"
#include <string>

// Represents a PyTypeObject in the debuggee's address space.
class RemotePyTypeObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyTypeObject(Offset objectAddress);

public: // Members.
	auto name() const -> std::string;
	auto documentation() const -> std::string;
	auto isPython2() const -> bool;
	auto repr(bool pretty = true) const -> std::string override;

};