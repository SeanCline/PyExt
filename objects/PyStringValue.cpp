#include "PyStringValue.h"

#include <string>
using namespace std;

namespace {

	constexpr auto hexAlphabet = "0123456789ABCDEF";

	auto charToHex(char c) -> string {
		string s(2, '\0');
		s[0] = hexAlphabet[(c & 0x0F)];
		s[1] = hexAlphabet[(c >> 4) & 0x0F];
		return s;
	}


	auto escapeCharacter(char c) -> string {
		// Return known escape sequences.
		switch (c) {
			default: break;
			case '\\': return "\\\\"; //< Backslash(\)
			case '\'': return "\\'";  //< Single quote (')
			case '"':  return "\\\""; //< Double quote (")
			case '\a': return "\\a";  //< ASCII Bell(BEL)
			case '\b': return "\\b";  //< ASCII Backspace(BS)
			case '\f': return "\\f";  //< ASCII Formfeed(FF)
			case '\n': return "\\n";  //< ASCII Linefeed(LF)
			case '\r': return "\\r";  //< ASCII Carriage Return(CR)
			case '\t': return "\\t";  //< ASCII Horizontal Tab(TAB)
			case '\v': return "\\v";  //< ASCII Vertical Tab(VT)
		}

		// Escape nonprintalbles with a hex code.
		if (!isprint(static_cast<unsigned char>(c))) {
			return "\\x"s + charToHex(c);
		}

		// Otherwise, it doesn't need escaped.
		return { c };
	}

}


auto PyStringValue::escapeAndQuoteString(const string& in, QuoteType quoteType) -> string
{
	string out;
	out.reserve(in.length() + 2);

	out += (quoteType == QuoteType::Double) ? '"' : '\'';
	for (auto c : in) {
		if ((quoteType == QuoteType::Single && c == '"')
			|| (quoteType == QuoteType::Double && c == '\''))
		{
			out += c; //< Don't escape the other kind of quote.
		} else {
			out += escapeCharacter(c);
		}
	}
	out += (quoteType == QuoteType::Double) ? '"' : '\'';

	return out;
}
