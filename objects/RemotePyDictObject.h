#pragma once

#include "RemotePyObject.h"

#include <memory>
#include <string>


// Represents a PyDictObject in the debuggee's address space.
class RemotePyDictObject : public RemotePyObject
{

public: // Construction/Destruction.
	RemotePyDictObject(Offset objectAddress);

public: // Members.
	SSize numUsed() const;
	SSize numMask() const;
	std::unique_ptr<RemotePyObject> at(SSize index) const;
	std::string repr(bool pretty = true) const override;

};