#include "catch_amalgamated.hpp"

#include "TestConfigData.h"

// Disable warnings from the Debugging Tools for Windows headers.
#pragma warning( push )
#pragma warning( disable : 4838 )
#pragma warning( disable : 5040 )
#include <engextcpp.hpp>
#pragma warning( pop )

// Provide a pretty-printer for the ExtException used by EngExtCpp.
CATCH_TRANSLATE_EXCEPTION(const ExtException& ex)
{
	return ex.GetMessage();
}


// Provide our own main as explained here:
// https://github.com/catchorg/Catch2/blob/devel/docs/own-main.md
// This lets us parse a couple extra command line arguments before forwarding to Catch.
int main(int argc, char* argv[])
{
	auto& configData = TestConfigData::instance();

	// Consume our custom args and collect the rest for Catch.
	std::vector<const char*> catchArgs;
	catchArgs.push_back(argv[0]);

	for (int i = 1; i < argc; ) {
		std::string_view arg(argv[i]);
		if (arg == "--object-types-dump-file" && i + 1 < argc) {
			configData.objectTypesDumpFileName = argv[i + 1];
			i += 2;
		} else if (arg == "--fibonacci-dump-file" && i + 1 < argc) {
			configData.fibonacciDumpFileName = argv[i + 1];
			i += 2;
		} else if (arg == "--object-details-dump-file" && i + 1 < argc) {
			configData.objectDetailsDumpFileName = argv[i + 1];
			i += 2;
		} else if (arg == "--localsplus-dump-file" && i + 1 < argc) {
			configData.localsplusDumpFileName = argv[i + 1];
			i += 2;
		} else if (arg == "--pystack-all-dump-file" && i + 1 < argc) {
			configData.pystackAllDumpFileName = argv[i + 1];
			i += 2;
		} else if (arg == "--symbol-path" && i + 1 < argc) {
			configData.symbolPath = argv[i + 1];
			i += 2;
		} else {
			catchArgs.push_back(argv[i++]);
		}
	}

	Catch::Session session;
	return session.run(static_cast<int>(catchArgs.size()), catchArgs.data());
}
