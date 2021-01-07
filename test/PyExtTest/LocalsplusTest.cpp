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
#include <string>
#include <algorithm>
#include <iterator>
#include <regex>

TEST_CASE("localsplus_test.py has the expected frames with localsplus.", "[integration][localsplus_test]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().localsplusDumpFileNameOrDefault());

	// Set up pyext.dll so it thinks DbgEng is calling into it.
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	std::vector<PyFrameObject> frames = dump.getMainThreadFrames();
	REQUIRE(frames.size() > 3);

	SECTION("Localsplus for frame in methode f().")
	{
		auto frameInF = std::find_if(begin(frames), end(frames), [](PyFrameObject& frame) {
			return frame.code()->name() == "f";
		});

		std::regex expectedRegex(
			R"(localsplus: \{\n)"
			R"(\t'self': <link cmd="!pyobj 0n\d+">&lt;A object&gt;</link>,\n)"
			R"(\t'a': 'test',\n)"
			R"(\t'b': \[ 1, 2, 3 \],\n)"
			R"(\t'c': \{\n)"
			R"(\t1: 2,\n)"
			R"(\t3: 4,\n)"
			R"(\},\n\})");
		REQUIRE(regex_match(frameInF->details(), expectedRegex));
	}
}