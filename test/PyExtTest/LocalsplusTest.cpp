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
using namespace std;

TEST_CASE("localsplus_test.py has the expected frames with localsplus.", "[integration][localsplus_test]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().localsplusDumpFileNameOrDefault());

	// Set up pyext.dll so it thinks DbgEng is calling into it.
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto frames = dump.getMainThreadFrames();
	REQUIRE(frames.size() > 6);

	vector<vector<string>> expectations{
		{
			"f_cellvar",

			R"(localsplus: \{\n)"
			R"(\t'self': <link cmd="!pyobj 0n\d+">&lt;A object&gt;</link>,\n)"
			R"(\t'param1': 'test',\n)"
			R"(\t'param2': \[ 1, 2, 3 \],\n)"
			R"(\t'local1': 5,\n)"
			R"(\t'f_cellfreevar': <link cmd="!pyobj 0n\d+">&lt;function f_cellfreevar&gt;</link>,\n)"
			R"(\t'cell1': <link cmd="!pyobj 0n\d+">&lt;cell object&gt;</link>: \{\n)"
			R"(\t1: 2,\n)"
			R"(\t3: 4,\n)"
			R"(\},\n\})"
		},
		{
			"f_cellfreevar",

			R"(localsplus: \{\n)"
			R"(\t'param': 7,\n)"
			R"(\t'local2': 6,\n)"
			R"(\t'f_freevar': <link cmd="!pyobj 0n\d+">&lt;function f_freevar&gt;</link>,\n)"
			R"(\t'cell2': <link cmd="!pyobj 0n\d+">&lt;cell object&gt;</link>: 9,\n)"
			R"(\t'cell1': <link cmd="!pyobj 0n\d+">&lt;cell object&gt;</link>: \{\n)"
			R"(\t1: 2,\n)"
			R"(\t3: 4,\n)"
			R"(\},\n\})"
		},
		{
			"f_freevar",

			R"(localsplus: \{\n)"
			R"(\t'x': 9,\n)"
			R"(\t'cell2': <link cmd="!pyobj 0n\d+">&lt;cell object&gt;</link>: 9,\n)"
			R"(\})"
		},
	};

	for (auto& exp : expectations) {
		auto& name = exp[0];
		auto& details = exp[1];

		SECTION("Localsplus for frame in method " + name + "().")
		{
			auto frame = std::find_if(begin(frames), end(frames), [&name](auto frame) {
				return frame->code()->name() == name;
				});

			std::regex expectedRegex(details);
			REQUIRE(regex_match((*frame)->details(), expectedRegex));
		}
	}
	
}