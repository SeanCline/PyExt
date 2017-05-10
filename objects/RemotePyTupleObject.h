#pragma once

#include "RemotePyVarObject.h"

#include <memory>
#include <string>


// Represents a RemotePyTupleObject in the debuggee's address space.
class RemotePyTupleObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyTupleObject(Offset objectAddress);

public: // Members.
	SSize numItems() const;
	std::unique_ptr<RemotePyObject> at(SSize index) const;
	std::string repr(bool pretty = true) const override;

};