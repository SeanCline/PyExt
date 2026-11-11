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
		/// The raw bytes for byte strings, or the UTF-8 encoded text for unicode strings.
		virtual auto stringValue() const -> std::string = 0;

	protected: // Helpers forimplementors of PyStringValue.
		enum class QuoteType { Single, Double };
		/// `escapeNonAscii` should be true for raw bytes and false for text that is already UTF-8 encoded.
		static auto escapeAndQuoteString(const std::string& str, QuoteType quoteType = QuoteType::Single, bool escapeNonAscii = true) -> std::string;
	};

}