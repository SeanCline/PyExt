#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "TestConfigData.h"

// Disable warnings from the Debugging Tools for Windows headers.
#pragma warning( push )
#pragma warning( disable : 4838 )
#pragma warning( disable : 5040 )
#include <engextcpp.hpp>
#pragma warning( pop ) 

// Provide a pretty-printer for the ExtException used by EngExtCpp.
CATCH_TRANSLATE_EXCEPTION(ExtException& ex)
{
	return ex.GetMessage();
}


// Provide our own main as explained here:
// https://github.com/catchorg/Catch2/blob/master/docs/own-main.md
// This lets us parse a couple extra command line arguments.
int main(int argc, char* argv[])
{
	auto& configData = TestConfigData::instance();

	// Add our own command line argments.
	using namespace Catch::clara;
	Catch::Session session;
	auto cli = session.cli();

	cli |= Opt(configData.objectTypesDumpFileName, "objectTypesDumpFileName")
		["--object-types-dump-file"]
		("The dump file to run tests object - types test against.");

	cli |= Opt(configData.fibonacciDumpFileName, "fibonaciiDumpFileName")
		["--fibonacci-dump-file"]
		("The dump file to run tests fibonacci tests against.");

	cli |= Opt(configData.symbolPath, "symbolPath")
		["--symbol-path"]
		("Location to look for Python symbols. (PDB files)");

	// Send our command line changes back into Catch.
	session.cli(cli);

	int returnCode = session.applyCommandLine(argc, argv);
	if (returnCode != 0) // Indicates a command line error
		return returnCode;

	// Run the tests!
	return session.run();
}