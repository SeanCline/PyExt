#include "extension.h"

#include <engextcpp.hpp>

#include <string>
#include <stdexcept>
#include <cassert>
using namespace std;


namespace {

	auto getSymbolPath(IDebugSymbols* pSymbols) -> string
	{
		// Get the size we'll need first.
		ULONG pathSize = 0;
		HRESULT hr = pSymbols->GetSymbolPath(nullptr, 0, &pathSize);
		if (FAILED(hr))
			throw runtime_error("GetSymbolPath failed to get path size with hr=" + hr);

		// Now get the buffer.
		string buff(pathSize, '\0');
		hr = pSymbols->GetSymbolPath(buff.data(), pathSize, &pathSize);
		if (FAILED(hr))
			throw runtime_error("GetSymbolPath failed to get path size with hr=" + hr);

		assert(pathSize == buff.size() && "Unexpected symbol path size after second call to GetSymbolPath.");
		buff.erase(buff.find('\0')); //< Trim off any extra from our string.

		return buff;
	}


	auto setSymbolPath(IDebugSymbols* pSymbols, const string& path) -> void
	{
		HRESULT hr = pSymbols->SetSymbolPath(path.c_str());
		if (FAILED(hr))
			throw runtime_error("SetSymbolPath failed with hr=" + hr);
	}

}


namespace PyExt {

	EXT_COMMAND(pysymfix, "Adds python symbols to the symbol path.", "")
	{
		auto sympath = getSymbolPath(m_Symbols);
		Out("Current symbol path: %s\n", sympath.c_str());

		if (sympath.find("pythonsymbols.sdcline.com/symbols") == string::npos) {
			Out("Adding symbol server to path...\n");
			setSymbolPath(m_Symbols, sympath + "srv*http://pythonsymbols.sdcline.com/symbols");
			Out("New symbol path: %s\n", getSymbolPath(m_Symbols).c_str());
		} else {
			Out("Python symbol server already in path.\n");
		}

		Out("Loading symbols...\n");
		ensureSymbolsLoaded();
	}

}
