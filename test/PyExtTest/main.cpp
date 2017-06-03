#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "TestConfigData.h"

// Disable warnings from the Debugging Tools for Windows headers.
#pragma warning( push )
#pragma warning( disable : 4838 )
#include <engextcpp.hpp>
#pragma warning( pop ) 

// Provide a pretty-printer for the ExtException used by EngExtCpp.
CATCH_TRANSLATE_EXCEPTION(ExtException& ex)
{
	return ex.GetMessageA();
}


// Provide our own main as explained here:
// https://github.com/philsquared/Catch/blob/master/docs/own-main.md
// This lets us parse a couple extra command line arguments.
int main(int argc, char* argv[])
{
	// Let Catch parse the command line first.
	Catch::Session session;
	int returnCode = session.applyCommandLine(argc, argv, Catch::Session::OnUnusedOptions::Ignore);
	if (returnCode != 0) // Indicates a command line error
		return returnCode;

	// Now we can parse our own command line options.
	Catch::Clara::CommandLine<TestConfigData> cli;

	cli["--object-types-dump-file"]
		.describe("The dump file to run tests object-types test against.")
		.bind(&TestConfigData::objectTypesDumpFileName, "objectTypesDumpFileName");

	cli["--fibonacci-dump-file"]
		.describe("The dump file to run tests fibonacci tests against.")
		.bind(&TestConfigData::fibonacciDumpFileName, "fibonaciiDumpFileName");

	cli["--symbol-path"]
		.describe("Location to look for Python symbols. (PDB files)")
		.bind(&TestConfigData::symbolPath, "symbolPath");

	cli.parseInto(Catch::Clara::argsToVector(argc, argv), TestConfigData::instance());

	// Run the tests!
	int numFailed = session.run();

	// Clamp to prevent false negatives.
	return (numFailed < 0xff ? numFailed : 0xff);
}