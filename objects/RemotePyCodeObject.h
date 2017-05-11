#pragma once

#include "RemotePyObject.h"
#include <string>


// Represents a PyFrameObject in the debuggee's address space.
class RemotePyCodeObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyCodeObject(Offset objectAddress);

public: // Members.
	int firstlineno() const;
	std::string filename() const;
	std::string name() const;
	
	std::string repr(bool pretty = true) const override;

};