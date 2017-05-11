#pragma once

#include "RemotePyVarObject.h"
#include <string>
#include <memory>

class RemotePyDictObject;
class RemotePyCodeObject;

// Represents a PyFrameObject in the debuggee's address space.
class RemotePyFrameObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyFrameObject(Offset objectAddress);

public: // Members.
	std::unique_ptr<RemotePyDictObject> locals() const;
	std::unique_ptr<RemotePyDictObject> globals() const;
	std::unique_ptr<RemotePyDictObject> builtins() const;
	std::unique_ptr<RemotePyCodeObject> code() const;
	std::unique_ptr<RemotePyFrameObject> back() const;
	std::string repr(bool pretty = true) const override;

};