#pragma once

#include "pyextpublic.h"

#include <string>
#include <cctype>

namespace PyExt::Remote {

	// Interface for objects that can provide a string representation of their value.
	// Intended for PyStringObject and PyUnicodeObject.
	class PYEXT_PUBLIC PyStringValue
	{

	public: // Construction/Destruction.
		virtual ~PyStringValue() = default;

	public: // Members.
		virtual auto stringValue() const -> std::string = 0;

	protected: // Helpers forimplementors of PyStringValue.
		enum class QuoteType { Single, Double };
		static auto escapeAndQuoteString(const std::string& str, QuoteType quoteType = QuoteType::Single) -> std::string;
	};

}