#include "ExtHelpers.h"

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
		if (codePoint < 0x80) {
			out += static_cast<char>(codePoint);
		} else if (codePoint < 0x800) {
			out += static_cast<char>(0xC0 | (codePoint >> 6));
			out += static_cast<char>(0x80 | (codePoint & 0x3F));
		} else if (codePoint < 0x10000) {
			out += static_cast<char>(0xE0 | (codePoint >> 12));
			out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (codePoint & 0x3F));
		} else {
			out += static_cast<char>(0xF0 | ((codePoint >> 18) & 0x07));
			out += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
			out += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (codePoint & 0x3F));
		}
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


	auto utf8ToAnsi(string_view utf8) -> string
	{
		const auto wide = utf8ToWide(utf8);
		if (wide.empty())
			return { };

		auto wideSize = static_cast<int>(wide.size());
		auto ansiSize = ::WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, nullptr, 0, nullptr, nullptr);
		string ansi(static_cast<size_t>(ansiSize), '\0');
		::WideCharToMultiByte(CP_ACP, 0, wide.data(), wideSize, ansi.data(), ansiSize, nullptr, nullptr);
		return ansi;
	}

}
