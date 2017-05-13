#pragma once

#include "RemotePyVarObject.h"
#include <string>

// Represents a PyStringObject in the debuggee's address space.
class RemotePyStringObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyStringObject(Offset objectAddress);

public: // Members.
	auto stringLength() const -> SSize;
	auto stringValue() const -> std::string;
	auto repr(bool pretty = true) const -> std::string override;

};