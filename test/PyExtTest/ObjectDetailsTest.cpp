#include "Catch.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"

#include <globals.h>
#include <PyStringValue.h>
#include <PyFrameObject.h>
#include <PyCodeObject.h>
#include <PyTypeObject.h>
#include <PyDictObject.h>
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
#include <regex>
using namespace std;

namespace {
	/// Finds a value in the given dict for a given key.
	template <typename RangeT>
	auto findValueByKey(RangeT& pairRange, const string& key) -> PyObject& {
		auto it = find_if(begin(pairRange), end(pairRange), [&](const auto& keyValuePair) {
			const auto* keyObj = dynamic_cast<PyStringValue*>(keyValuePair.first.get());
			return keyObj != nullptr && keyObj->stringValue() == key;
		});

		if (it == end(pairRange))
			throw runtime_error("Value not found for key=" + key);
		return *it->second;
	}
}


TEST_CASE("object_details.py has a stack frame with expected locals.", "[integration][object_details]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectDetailsDumpFileNameOrDefault());

	// Set up pyext.dll so it thinks DbgEng is calling into it.
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	vector<PyFrameObject> frames = dump.getMainThreadFrames();
	auto& bottomFrame = frames.back();
	REQUIRE(bottomFrame.code()->name() == "<module>");

	auto locals = bottomFrame.locals();
	REQUIRE(locals != nullptr);

	auto localPairs = locals->pairValues();
	REQUIRE(localPairs.size() >= 14);


	SECTION("Details of objects.")
	{
		vector<vector<int>> test { { 1, 2}, {3, 4} };
		vector<vector<string>> expectations{
			{ "d"     , "D"     , "dict: {\n\t'd1': 1,\n\t'd2': 2,\n}" },
			{ "s"     , "S"     , "slots: {\n\tslot1: 1,\n\tslot2: 2,\n}" },
			{ "dsubd" , "DsubD" , "dict: {\n\t'd1': 1,\n\t'd2': 2,\n\t'd3': 3,\n}" },
			{ "ssubs" , "SsubS" , "slots: {\n\tslot3: 3,\n\tslot1: 1,\n\tslot2: 2,\n}" },
			{ "dsubs" , "DsubS" , "slots: {\n\tslot1: 1,\n\tslot2: 2,\n}\ndict: {\n\t'd3': 3,\n}" },
			{ "ssubd" , "SsubD" , "slots: {\n\tslot3: 3,\n}\ndict: {\n\t'd1': 1,\n\t'd2': 2,\n}" },
			{ "ssubds", "SsubDS", "slots: {\n\tslot3: 5,\n\tslot1: 3,\n\tslot2: 4,\n}\ndict: {\n\t'd1': 1,\n\t'd2': 2,\n}" },
		};
		for (auto& objExp : expectations) {
			auto& name = objExp[0];
			auto& expectedType = objExp[1];
			auto& expectedDetails = objExp[2];

			auto& obj = findValueByKey(localPairs, name);
			REQUIRE(obj.type().name() == expectedType);
			REQUIRE(obj.details() == expectedDetails);
		}
	}

}