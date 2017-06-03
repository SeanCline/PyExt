#include "Catch.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"

#include <globals.h>
#include <PyInterpreterState.h>
#include <PyThreadState.h>
#include <PyFrameObject.h>
#include <PyCodeObject.h>
#include <PyTypeObject.h>
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include <regex>

TEST_CASE("fibonacci_test.py has the expected line numbers.", "[integration][fibonacci_test]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().fibonaciiDumpFileNameOrDefault());

	// Set up pyext.dll so it thinks DbgEng is calling into it.
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	std::vector<PyFrameObject> frames = dump.getMainThreadFrames();
	REQUIRE(frames.size() > 90);

	SECTION("Bottom frame is the module.")
	{
		auto& bottomFrame = frames.back();
		REQUIRE(bottomFrame.type().name() == "frame");
		REQUIRE(bottomFrame.currentLineNumber() == 28);

		auto codeObj = bottomFrame.code();
		REQUIRE(codeObj != nullptr);
		REQUIRE(codeObj->name() == "<module>");
		REQUIRE(codeObj->filename().find("fibonacci_test.py") != std::string::npos);

		// Expected to be similar to: (<code object, file ".\fibonacci_test.py", line 9>)
		std::regex expectedRegex(R"(<code object, file "[^"]*fibonacci_test.py", line 9>)");
		REQUIRE(regex_match(codeObj->repr(false), expectedRegex));
		REQUIRE(regex_match(codeObj->repr(true), expectedRegex));
	}

	SECTION("The next several frames are in function recursive_fib.")
	{
		auto numFibFrames = std::count_if(begin(frames), end(frames), [](PyFrameObject& frame) {
			return frame.code()->name() == "recursive_fib" && frame.currentLineNumber() == 24;
		});

		REQUIRE(numFibFrames > 90);
	}

	SECTION("The top frame in recursive_fib is the one that triggered the dump.")
	{
		auto topFrameInFib = std::find_if(begin(frames), end(frames), [](PyFrameObject& frame) {
			return frame.code()->name() == "recursive_fib";
		});

		REQUIRE(topFrameInFib->currentLineNumber() == 18);
		REQUIRE((topFrameInFib - 1)->code()->name() == "dump_process");
	}
}