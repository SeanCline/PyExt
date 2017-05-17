#pragma once

#include <string>
#include <cctype>

// Interface for objects that can provide a string representation of their value.
// Intended for RemotePyStringObject and RemotePyUnicodeObject.
class PyStringValue
{

public: // Construction/Destruction.
	virtual ~PyStringValue() = default;

public: // Members.
	virtual auto stringValue() const -> std::string = 0;

protected: // Helpers forimplementors of PyStringValue.
	enum class QuoteType { Single, Double };
	static auto escapeAndQuoteString(const std::string& str, QuoteType quoteType = QuoteType::Single) -> std::string;
};