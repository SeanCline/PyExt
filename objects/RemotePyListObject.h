#pragma once

#include "RemotePyVarObject.h"

#include <memory>
#include <string>


// Represents a RemotePyListObject in the debuggee's address space.
class RemotePyListObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyListObject(Offset objectAddress);

public: // Members.
	auto numItems() const -> SSize;
	auto at(SSize index) const -> std::unique_ptr<RemotePyObject>;
	auto repr(bool pretty = true) const -> std::string override;

};