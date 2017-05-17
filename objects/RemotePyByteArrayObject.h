#pragma once

#include "RemotePyVarObject.h"
#include "PyStringValue.h"
#include <string>
#include <vector>

// Represents a PyStringObject or a PyBytesObject in the debuggee's address space.
class RemotePyByteArrayObject : public RemotePyVarObject, public PyStringValue
{

public: // Construction/Destruction.
	explicit RemotePyByteArrayObject(Offset objectAddress);

public: // Members.
	auto arrayValue() const -> std::vector<uint8_t>;
	auto stringLength() const -> SSize;
	auto stringValue() const -> std::string override;
	auto repr(bool pretty = true) const -> std::string override;

};
