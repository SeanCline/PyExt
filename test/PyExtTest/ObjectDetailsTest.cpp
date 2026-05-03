#include "catch_amalgamated.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"
#include "TestHelpers.h"

#include <globals.h>
#include <PyFrameObject.h>
#include <PyCodeObject.h>
#include <PyTypeObject.h>
#include <PyDictObject.h>
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>
#include <vector>
#include <string>
#include <regex>
using namespace std;


TEST_CASE("object_details.py has a stack frame with expected locals.", "[integration][object_details]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectDetailsDumpFileNameOrDefault());

	// Set up pyext.dll so it thinks DbgEng is calling into it.
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto frames = dump.getMainThreadFrames();
	auto bottomFrame = frames.back();
	REQUIRE(bottomFrame->code()->name() == "<module>");

	auto locals = bottomFrame->locals();
	REQUIRE(locals != nullptr);

	auto localPairs = locals->pairValues();
	REQUIRE(localPairs.size() >= 14);


	vector<vector<string>> expectations{
		// Regex only necessary due to Python 2 (dicts not sorted)
		{ "d"            , "D"                  , R"(dict: \{\n\t'(d1': 1,\n\t'd2': 2,|d2': 2,\n\t'd1': 1,)\n\})" },
		{ "s"            , "S"                  , R"(slots: \{\n\tslot1: 1,\n\tslot2: 2,\n\})" },
		{ "dsubd"        , "DsubD"              , R"(dict: \{\n(\t('d1': 1|'d2': 2|'d3': 3),\n){3}\})" },
		{ "ssubs"        , "SsubS"              , R"(slots: \{\n\tslot3: 3,\n\tslot1: 1,\n\tslot2: 2,\n\})" },
		{ "dsubs"        , "DsubS"              , R"(slots: \{\n\tslot1: 1,\n\tslot2: 2,\n\}\ndict: \{\n\t'd3': 3,\n\})" },
		{ "ssubd"        , "SsubD"              , R"(slots: \{\n\tslot3: 3,\n\}\ndict: \{\n(\t('d1': 1|'d2': 2),\n){2}\})" },
		{ "ssubds"       , "SsubDS"             , R"(slots: \{\n\tslot3: 5,\n\tslot1: 3,\n\tslot2: 4,\n\}\ndict: \{\n(\t('d1': 1|'d2': 2),\n){2}\})" },
		{ "negDictOffset", "NegDictOffset"      , R"(tuple repr: \(1, 2, 3\)\ndict: \{\n\t'attr': 'test',\n\})" },
		{ "manDictRes"   , "ManagedDictResolved", R"(dict: \{(\n\t'a\d+': \d+,){32}\n\})" },
	};

	for (auto& objExp : expectations) {
		auto& name = objExp[0];
		auto& expectedType = objExp[1];
		auto& expectedDetails = objExp[2];

		SECTION("Details of " + name + " object")
		{	
			auto& obj = TestHelpers::findValueByKey(localPairs, name);
			REQUIRE(obj.type().name() == expectedType);
			std::regex expectedRegex(expectedDetails);
			REQUIRE(regex_match(obj.details(), expectedRegex));
		}
	}

}
