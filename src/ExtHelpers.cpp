#include "ExtHelpers.h"

#include <string>
#include <string_view>
#include <sstream>
using namespace std;


namespace utils {

	auto getPointerSize() -> int
	{
		string objExpression = "sizeof(void*)"s;
		ExtRemoteTyped remoteObj(objExpression.c_str());
		return utils::readIntegral<int>(remoteObj);
	}


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


	auto getFullSymbolName(string_view symbolName) -> string
	{
		// Locate the module through a symbol it defines rather than by guessing at the module's
		// name, which varies ("python311", "python315t", "python315_d", ...).
		// `_PyRuntime` (Python 3.7+) is used because it isn't part of the stable ABI. Stable ABI
		// symbols are also exported from python3.dll as forwarders, and could resolve to that instead.
		string moduleName;
		ULONG64 symbolOffset = 0, moduleBase = 0;
		ULONG moduleIndex = 0;
		char nameBuffer[MAX_PATH] = {};
		if (SUCCEEDED(g_Ext->m_Symbols->GetOffsetByName("_PyRuntime", &symbolOffset))
			&& SUCCEEDED(g_Ext->m_Symbols->GetModuleByOffset(symbolOffset, 0, &moduleIndex, &moduleBase))
			&& SUCCEEDED(g_Ext->m_Symbols->GetModuleNames(moduleIndex, moduleBase,
				nullptr, 0, nullptr, nameBuffer, sizeof(nameBuffer), nullptr, nullptr, 0, nullptr)))
		{
			moduleName = nameBuffer;
		} else {
			ExtBuffer<char> buffer;
			g_Ext->FindFirstModule("python???", &buffer, 0);
			moduleName = buffer.GetBuffer();
		}

		string result = moduleName;
		result += '!';
		result += symbolName;
		return result;
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


	auto pythonVersionHex() -> optional<uint32_t>
	{
		optional<uint32_t> version;
		ignoreExtensionError([&] {
			ExtRemoteTyped remoteVersion("Py_Version");
			version = readIntegral<uint32_t>(remoteVersion);
		});
		return version;
	}

}
