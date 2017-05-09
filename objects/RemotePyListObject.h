#pragma once

#include "RemotePyVarObject.h"

#include <memory>
#include <string>


// Represents a RemotePyListObject in the debuggee's address space.
class RemotePyListObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	RemotePyListObject(Offset objectAddress);

public: // Members.
	SSize numItems() const;
	std::unique_ptr<RemotePyObject> at(SSize index) const;
	std::string repr(bool pretty = true) const override;

};