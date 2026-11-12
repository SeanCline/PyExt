#include "ExtHelpers.h"

#include <climits>
#include <cuchar>
#include <string>
#include <string_view>
#include <sstream>
using namespace std;


namespace utils {

	auto escapeDml(string_view str) -> string
	{
		std::string buffer;
		buffer.reserve(str.size());
		for (auto ch : str) {
			switch (ch) {
			case '&':  buffer += "&amp;";  break;
			case '\"': buffer += "&quot;"; break;
			// case '\'': buffer += "&apos;"; break; no DML special character?!
			case '<':  buffer += "&lt;";   break;
			case '>':  buffer += "&gt;";   break;
			default:   buffer += ch;       break;
			}
		}
		return buffer;
	}


	auto link(string_view text, string_view cmd, string_view alt) -> string
	{
		ostringstream oss;
		oss << "<link cmd=\"" << escapeDml(cmd) << "\"";
		if (!alt.empty())
			oss << " alt=\"" << escapeDml(alt) << "\"";
		oss << ">" << escapeDml(text) << "</link>";
		return oss.str();
	}


	auto appendUtf8(string& out, char32_t codePoint) -> void
	{
		// Microsoft's c32rtomb always produces UTF-8, whatever the locale.
		char utf8[MB_LEN_MAX];
		mbstate_t state{};
		auto length = c32rtomb(utf8, codePoint, &state);
		if (length == static_cast<size_t>(-1))
			out += "\xEF\xBF\xBD"; //< U+FFFD REPLACEMENT CHARACTER
		else
			out.append(utf8, length);
	}


	auto utf8ToWide(string_view utf8) -> wstring
	{
		if (utf8.empty())
			return { };

		auto inputSize = static_cast<int>(utf8.size());
		auto wideSize = ::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), inputSize, nullptr, 0);
		wstring wide(static_cast<size_t>(wideSize), L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, utf8.data(), inputSize, wide.data(), wideSize);
		return wide;
	}


	static auto wideToMultiByte(wstring_view wide, UINT codePage) -> string
	{
		if (wide.empty())
			return { };

		auto wideSize = static_cast<int>(wide.size());
		auto size = ::WideCharToMultiByte(codePage, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
		string result(static_cast<size_t>(size), '\0');
		::WideCharToMultiByte(codePage, 0, wide.data(), wideSize, result.data(), size, nullptr, nullptr);
		return result;
	}


	auto wideToUtf8(wstring_view wide) -> string
	{
		return wideToMultiByte(wide, CP_UTF8);
	}


	auto utf8ToAnsi(string_view utf8) -> string
	{
		return wideToMultiByte(utf8ToWide(utf8), CP_ACP);
	}

}
