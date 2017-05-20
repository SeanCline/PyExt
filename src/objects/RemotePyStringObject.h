#pragma once

#include "RemotePyVarObject.h"
#include "PyStringValue.h"
#include <string>

// Represents a PyStringObject or a PyBytesObject in the debuggee's address space.
class RemotePyBaseStringObject : public RemotePyVarObject, public PyStringValue
{

public: // Construction/Destruction.
	explicit RemotePyBaseStringObject(Offset objectAddress, const std::string& typeName);

public: // Members.
	auto stringLength() const -> SSize;
	auto stringValue() const -> std::string override;
	auto repr(bool pretty = true) const -> std::string override;

};

// Python3 has a PyBytesObject that takes the place of PyStringObject.
// It shares the same layout as Python2's PyStringObject but has a different symbol name.

// Represents a Python3 PyBytesObject in the debuggee's address space.
class RemotePyBytesObject : public RemotePyBaseStringObject {
public:
	explicit RemotePyBytesObject(Offset objectAddress);
};


// Represents a Python2 PyStringObject in the debuggee's address space.
class RemotePyStringObject : public RemotePyBaseStringObject
{
public:
	explicit RemotePyStringObject(Offset objectAddress);
};