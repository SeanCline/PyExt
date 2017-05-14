#pragma once

#include "RemotePyVarObject.h"

#include <memory>
#include <string>
#include <vector>

// Represents a RemotePyTupleObject in the debuggee's address space.
class RemotePyTupleObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyTupleObject(Offset objectAddress);

public: // Members.
	auto numItems() const -> SSize;
	auto at(SSize index) const -> std::unique_ptr<RemotePyObject>;
	auto listValue() const->std::vector<std::unique_ptr<RemotePyObject>>;
	auto repr(bool pretty = true) const -> std::string override;

};