#pragma once

#include <string>

// Interface for objects that can provide a string representation of their value.
// Intended for RemotePyStringObject and RemotePyUnicodeObject.
class PyStringValue
{

public: // Construction/Destruction.
	virtual ~PyStringValue() = default;

public: // Members.
	virtual auto stringValue() const -> std::string = 0;

};