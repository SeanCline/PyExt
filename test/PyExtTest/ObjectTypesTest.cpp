#include "catch_amalgamated.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"
#include "TestHelpers.h"

#include <globals.h>
#include <PyStringValue.h>
#include <PyFrameObject.h>
#include <PyCodeObject.h>
#include <PyTypeObject.h>
#include <PyDictObject.h>
#include <PyIntObject.h>
#include <PyLongObject.h>
#include <PyFloatObject.h>
#include <PyComplexObject.h>
#include <PyFunctionObject.h>
#include <PyListObject.h>
#include <PyTupleObject.h>
#include <PySetObject.h>
#include <PyUnicodeObject.h>
#include <PyGenObject.h>
#include <PyWeakrefObject.h>
#include <PyCellObject.h>
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>
#include <vector>
#include <string>
#include <complex>
#include <regex>
using namespace std;


TEST_CASE("object_types.py has a stack frame with expected locals.", "[integration][object_types]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectTypesDumpFileNameOrDefault());

	// Set up pyext.dll so it thinks DbgEng is calling into it.
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto frames = dump.getMainThreadFrames();
	auto bottomFrame = frames.back();
	auto bottomCode = bottomFrame->code();
	REQUIRE(bottomCode != nullptr);
	REQUIRE(bottomCode->name() == "<module>");

	auto locals = bottomFrame->locals();
	REQUIRE(locals != nullptr);

	auto localPairs = locals->pairValues();
	REQUIRE(localPairs.size() >= 17);


	SECTION("Value of string_obj.")
	{
		const string expectedValue = "TestString123";

		auto& string_obj = TestHelpers::findValueByKey(localPairs, "string_obj");
		REQUIRE(string_obj.type().name() == "str");
		REQUIRE(string_obj.repr().find(expectedValue) != string::npos);
		REQUIRE(string_obj.details() == "");

		auto stringValue = dynamic_cast<PyStringValue&>(string_obj).stringValue();
		REQUIRE(stringValue == expectedValue);
	}


	SECTION("Value of big_string_obj.")
	{
		string expectedValue;
		for (int i = 0; i < 100; ++i)
			expectedValue += "TestString123";

		auto& big_string_obj = TestHelpers::findValueByKey(localPairs, "big_string_obj");
		REQUIRE(big_string_obj.type().name() == "str");
		REQUIRE(big_string_obj.repr().find(expectedValue) != string::npos);
		REQUIRE(big_string_obj.details() == "");

		auto stringValue = dynamic_cast<PyStringValue&>(big_string_obj).stringValue();
		REQUIRE(stringValue == expectedValue);
	}


	SECTION("Value of bytes_obj.")
	{
		const string expectedValue = "TestBytes123";

		auto& bytes_obj = TestHelpers::findValueByKey(localPairs, "bytes_obj");
		auto typeName = bytes_obj.type().name();
		REQUIRE((typeName == "str" || typeName == "bytes"));
		REQUIRE(bytes_obj.repr().find(expectedValue) != string::npos);
		REQUIRE(bytes_obj.details() == "");

		auto stringValue = dynamic_cast<PyStringValue&>(bytes_obj).stringValue();
		REQUIRE(stringValue == expectedValue);
	}


	SECTION("Value of byte_array_object.")
	{
		const string expectedValue = "TestBytearray123";

		auto& byte_array_object = TestHelpers::findValueByKey(localPairs, "byte_array_object");
		REQUIRE(byte_array_object.type().name() == "bytearray");
		REQUIRE(byte_array_object.repr() == "bytearray(b'"+ expectedValue + "')");
		REQUIRE(byte_array_object.details() == "");

		auto stringValue = dynamic_cast<PyStringValue&>(byte_array_object).stringValue();
		REQUIRE(stringValue == expectedValue);
	}


	SECTION("Value of int_obj.")
	{
		const int expectedValue = 1;

		auto& int_obj = TestHelpers::findValueByKey(localPairs, "int_obj");
		REQUIRE(int_obj.type().name() == "int");
		REQUIRE(int_obj.repr() == to_string(expectedValue));
		REQUIRE(int_obj.details() == "");

		if (dynamic_cast<PyIntObject*>(&int_obj) != nullptr) { //< PyIntObject only applies to Python2.
			const auto actualValue = dynamic_cast<PyIntObject&>(int_obj).intValue();
			REQUIRE(expectedValue == actualValue);
		}
	}


	SECTION("Value of long_obj.")
	{
		const string expectedValue = "123456789012345678901234567890123456789012345678901234567890";

		auto& long_obj = dynamic_cast<PyLongObject&>(TestHelpers::findValueByKey(localPairs, "long_obj"));
		const auto typeName = long_obj.type().name();
		REQUIRE((typeName == "int" || typeName == "long"));
		REQUIRE(long_obj.repr() == expectedValue);
		REQUIRE(long_obj.details() == "");
	}


	SECTION("Value of all_long_obj.")
	{
		const string expectedValue = "0,-123456,123456,987654321987654321987654321,-987654321987654321987654321";

		auto& tuple_obj = dynamic_cast<PyTupleObject&>(TestHelpers::findValueByKey(localPairs, "all_long_obj"));
		auto value = tuple_obj.repr(false);
		std::regex chars_to_remove("[\\s()]+");
		value = std::regex_replace(value, chars_to_remove, "");
		REQUIRE(value == expectedValue);
	}


	SECTION("Value of float_obj.")
	{
		const double expectedValue = 3.1415;

		auto& float_obj = dynamic_cast<PyFloatObject&>(TestHelpers::findValueByKey(localPairs, "float_obj"));
		REQUIRE(float_obj.type().name() == "float");
		REQUIRE(float_obj.repr() == "3.1415");
		REQUIRE(float_obj.details() == "");

		// Comparing floats can be error prone. If this fails, it might need some wiggle room in the compare.
		REQUIRE(float_obj.floatValue() == expectedValue);
	}


	SECTION("Value of complex_obj.")
	{
		const complex<double> expectedValue{1.5, -2.25};

		auto& complex_obj = dynamic_cast<PyComplexObject&>(TestHelpers::findValueByKey(localPairs, "complex_obj"));
		REQUIRE(complex_obj.type().name() == "complex");

		// Comparing floats can be error prone. If this fails, it might need some wiggle room in the compare.
		REQUIRE(complex_obj.complexValue() == expectedValue);

		// Expected to be similar to: (1.5+2.25j)
		std::regex expectedRegex(R"(\(\s*1.5(0)*\s*-\s*2.25(0)*\s*j\s*\))");
		auto c = complex_obj.repr(false);
		REQUIRE(regex_match(complex_obj.repr(false), expectedRegex));
		REQUIRE(regex_match(complex_obj.repr(true), expectedRegex));
		REQUIRE(complex_obj.details() == "");
	}


	SECTION("Value of bool_true_obj.")
	{
		auto& bool_true_obj = TestHelpers::findValueByKey(localPairs, "bool_true_obj");
		REQUIRE(bool_true_obj.type().name() == "bool");


		const auto actualValue = bool_true_obj.repr();
		REQUIRE(actualValue == "True");
		REQUIRE(bool_true_obj.details() == "");
	}


	SECTION("Value of bool_false_obj.")
	{
		auto& bool_false_obj = TestHelpers::findValueByKey(localPairs, "bool_false_obj");
		REQUIRE(bool_false_obj.type().name() == "bool");

		const auto actualValue = bool_false_obj.repr();
		REQUIRE(actualValue == "False");
		REQUIRE(bool_false_obj.details() == "");
	}


	SECTION("Value of none_obj.")
	{
		auto& none_obj = TestHelpers::findValueByKey(localPairs, "none_obj");
		REQUIRE(none_obj.type().name() == "NoneType");
		REQUIRE(none_obj.repr() == "None");
		REQUIRE(none_obj.details() == "");
	}


	SECTION("Value of type_obj.")
	{
		auto& type_obj = dynamic_cast<PyTypeObject&>(TestHelpers::findValueByKey(localPairs, "type_obj"));
		REQUIRE(type_obj.type().name() == "type");
		REQUIRE(type_obj.name() == "dict");
		REQUIRE(type_obj.repr(false) == "<class 'dict'>");
		// Expected to be similar to: "<link cmd="!pyobj 0n140729205561440">&lt;class 'dict'&gt;</link>"
		std::regex expectedRegex(R"(<link cmd="!pyobj 0n\d+">&lt;class 'dict'&gt;</link>)");
		REQUIRE(regex_match(type_obj.repr(), expectedRegex));
		expectedRegex = std::regex(R"(dict: \{[^]+\})");
		REQUIRE(regex_match(type_obj.details(), expectedRegex));
	}


	SECTION("Value of not_implemented_obj.")
	{
		auto& not_implemented_obj = TestHelpers::findValueByKey(localPairs, "not_implemented_obj");
		REQUIRE(not_implemented_obj.type().name() == "NotImplementedType");
		REQUIRE(not_implemented_obj.repr() == "NotImplemented");
		REQUIRE(not_implemented_obj.details() == "");
	}


	SECTION("Value of func_obj.")
	{
		auto& func_obj = dynamic_cast<PyFunctionObject&>(TestHelpers::findValueByKey(localPairs, "func_obj"));
		REQUIRE(func_obj.type().name() == "function");
		REQUIRE(func_obj.repr(false) == "<function test_function>");
		// Expected to be similar to: "<link cmd="!pyobj 0n140729205561440">&lt;function test_function&gt;</link>"
		std::regex expectedRegex(R"(<link cmd="!pyobj 0n\d+">&lt;function test_function&gt;</link>)");
		REQUIRE(regex_match(func_obj.repr(), expectedRegex));
		REQUIRE(func_obj.name()->stringValue() == "test_function");
		REQUIRE(func_obj.code() != nullptr);
		REQUIRE(func_obj.doc()->repr().find("Some DocString") != string::npos);
		REQUIRE(func_obj.details() == "");

		auto qualname = func_obj.qualname();
		if (qualname != nullptr) //< Python2 doesn't have qualified names.
			REQUIRE(qualname->stringValue() == "test_function");
	}


	SECTION("Value of list_obj.")
	{
		auto& list_obj = dynamic_cast<PyListObject&>(TestHelpers::findValueByKey(localPairs, "list_obj"));
		REQUIRE(list_obj.type().name() == "list");
		REQUIRE(list_obj.numItems() == 3);

		// Individual elements.
		REQUIRE_THROWS(list_obj.at(-1));
		REQUIRE(list_obj.at(0)->type().name() == "str");
		REQUIRE(list_obj.at(1)->type().name() == "int");
		REQUIRE_NOTHROW(list_obj.at(2));
		REQUIRE_THROWS(list_obj.at(3));

		// Expected to be similar to: [ 'TestString123', 1, 123456789012345678901234567890123456789012345678901234567890 ]
		std::regex expectedRegex(R"(\[\s*'TestString123',\s*1,\s*123456789012345678901234567890123456789012345678901234567890,?\s*\])");
		REQUIRE(regex_match(list_obj.repr(false), expectedRegex));
		REQUIRE(regex_match(list_obj.repr(true), expectedRegex));
		REQUIRE(list_obj.details() == "");
	}


	SECTION("Value of tuple_obj.")
	{
		auto& tuple_obj = dynamic_cast<PyTupleObject&>(TestHelpers::findValueByKey(localPairs, "tuple_obj"));
		REQUIRE(tuple_obj.type().name() == "tuple");
		REQUIRE(tuple_obj.numItems() == 3);

		// Individual elements.
		REQUIRE_THROWS(tuple_obj.at(-1));
		REQUIRE(tuple_obj.at(0)->type().name() == "str");
		REQUIRE(tuple_obj.at(1)->type().name() == "int");
		REQUIRE_NOTHROW(tuple_obj.at(2));
		REQUIRE_THROWS(tuple_obj.at(3));

		// Expected to be similar to: ('TestString123', 1, 123456789012345678901234567890123456789012345678901234567890)
		std::regex expectedRegex(R"(\(\s*'TestString123',\s*1,\s*123456789012345678901234567890123456789012345678901234567890,?\s*\))");
		REQUIRE(regex_match(tuple_obj.repr(false), expectedRegex));
		REQUIRE(regex_match(tuple_obj.repr(true), expectedRegex));
		REQUIRE(tuple_obj.details() == "");
	}


	SECTION("Value of set_obj.")
	{
		auto& set_obj = dynamic_cast<PySetObject&>(TestHelpers::findValueByKey(localPairs, "set_obj"));
		REQUIRE(set_obj.type().name() == "set");
		REQUIRE(set_obj.numItems() == 3);
		REQUIRE(set_obj.listValue().size() == 3);

		// Expected to be similar to: {'TestString123', 1, 123456789012345678901234567890123456789012345678901234567890}
		std::regex expectedRegex(R"(\{(('TestString123'\s*)|(\d+)|\s*|,)+\})");
		REQUIRE(regex_match(set_obj.repr(false), expectedRegex));
		REQUIRE(regex_match(set_obj.repr(true), expectedRegex));
		REQUIRE(set_obj.details() == "");
	}


	SECTION("Value of dict_obj.")
	{
		auto& dict_obj = dynamic_cast<PyDictObject&>(TestHelpers::findValueByKey(localPairs, "dict_obj"));
		REQUIRE(dict_obj.type().name() == "dict");

		// Individual elements.
		auto pairs = dict_obj.pairValues();
		REQUIRE(pairs.size() == 3);

		// Expected to be similar to: { 'string_obj': 'TestString123', 'int_obj' : 1, 'long_obj' : 123456789012345678901234567890123456789012345678901234567890, }
		std::regex expectedRegex(R"(\{(('string_obj':\s*'TestString123')|('int_obj':\s*\d+)|('long_obj':\s*\d+)|\s*|,)+\})");
		REQUIRE(regex_match(dict_obj.repr(false), expectedRegex));
		REQUIRE(regex_match(dict_obj.repr(true), expectedRegex));
		REQUIRE(dict_obj.details() == "");
	}


	SECTION("Value of non-ASCII strings.")
	{
		// Each string is stored with a different width per character, and is expected back as UTF-8.
		vector<pair<string, string>> expectations{
			{ "unicode_latin1_obj", "caf\xC3\xA9" },
			{ "unicode_bmp_obj",    "\xE4\xB8\xAD\xE6\x96\x87" },
			{ "unicode_astral_obj", "smile \xF0\x9F\x98\x80" },
		};

		for (auto& [name, expectedValue] : expectations) {
			auto& obj = TestHelpers::findValueByKey(localPairs, name);
			auto* unicodeObj = dynamic_cast<PyUnicodeObject*>(&obj);
			if (unicodeObj == nullptr)
				continue; //< Python 2's `unicode` type isn't supported.

			INFO("Checking " << name);
			REQUIRE(unicodeObj->stringValue() == expectedValue);
			REQUIRE(unicodeObj->repr(false) == "'" + expectedValue + "'");

			// The header of these strings is nested differently than it is for ASCII strings.
			REQUIRE(unicodeObj->refCount() > 0);
			REQUIRE(unicodeObj->type().name() == "str");
		}
	}


	SECTION("Value of control_bytes_obj.")
	{
		auto& control_bytes_obj = TestHelpers::findValueByKey(localPairs, "control_bytes_obj");
		REQUIRE(control_bytes_obj.repr(false).find(R"('\x01\x1f\xab')") != string::npos);
	}


	SECTION("Value of floats that don't suit fixed-point notation.")
	{
		REQUIRE(TestHelpers::findValueByKey(localPairs, "small_float_obj").repr() == "1.5e-10");
		REQUIRE(TestHelpers::findValueByKey(localPairs, "whole_float_obj").repr() == "2.0");
	}


	SECTION("Value of single_tuple_obj.")
	{
		REQUIRE(TestHelpers::findValueByKey(localPairs, "single_tuple_obj").repr(false) == "(1,)");
	}


	SECTION("Value of empty and frozen sets.")
	{
		REQUIRE(TestHelpers::findValueByKey(localPairs, "empty_set_obj").repr(false) == "set()");

		std::regex expectedRegex(R"(frozenset\(\{\s*1,\s*\}\))");
		REQUIRE(regex_match(TestHelpers::findValueByKey(localPairs, "frozenset_obj").repr(false), expectedRegex));
	}


	SECTION("Reference count of an immortal object.")
	{
		// Python 3.12+ gives None a sentinel reference count with its high bits set,
		// which must not come back as a negative number.
		auto& none_obj = TestHelpers::findValueByKey(localPairs, "none_obj");
		REQUIRE(none_obj.refCount() > 0);
	}


	SECTION("State of generators and coroutines.")
	{
		vector<pair<string, string>> expectations{
			{ "generator_created_obj",  "created" },
			{ "generator_obj",          "suspended" },
			{ "generator_finished_obj", "cleared" },
			{ "coroutine_obj",          "created" },
		};

		for (auto& [name, expectedState] : expectations) {
			if (name == "coroutine_obj" && bottomCode->type().isPython2())
				continue; //< `async def` doesn't exist there, so the script couldn't make one.

			auto& genObj = dynamic_cast<PyGenObjectBase&>(TestHelpers::findValueByKey(localPairs, name));
			INFO("Checking " << name);

			// The numbering of the states has changed between versions, so every version is worth checking.
			if (genObj.frameState().has_value()) {
				REQUIRE(genObj.frameStateString() == expectedState);
				REQUIRE(genObj.repr(false).find("(" + expectedState + ")>") != string::npos);
			} else {
				REQUIRE(genObj.frameStateString() == ""); //< Python <= 3.10 keeps the state elsewhere.
			}
		}
	}


	SECTION("Value of weakref_obj.")
	{
		auto& weakref_obj = dynamic_cast<PyWeakrefObject&>(TestHelpers::findValueByKey(localPairs, "weakref_obj"));

		std::regex expectedRegex(R"(<\S+ to <Referent object>>)");
		REQUIRE(regex_match(weakref_obj.repr(false), expectedRegex));

		// The angle brackets must be escaped or the debugger takes them to be DML tags and prints nothing.
		expectedRegex = R"(&lt;\S+ to <link cmd="!pyobj 0n\d+">&lt;Referent object&gt;</link>&gt;)";
		REQUIRE(regex_match(weakref_obj.repr(true), expectedRegex));
	}


	SECTION("Value of empty_cell_obj.")
	{
		auto& empty_cell_obj = dynamic_cast<PyCellObject&>(TestHelpers::findValueByKey(localPairs, "empty_cell_obj"));
		REQUIRE(empty_cell_obj.objectReference() == nullptr);
		REQUIRE(empty_cell_obj.repr(false) == "<cell object>: <empty>");
	}


	SECTION("Value of frame_obj.")
	{
		// Python 3.11+ keeps a frame object's state in a separate interpreter frame.
		auto& frame_obj = dynamic_cast<PyFrameObject&>(TestHelpers::findValueByKey(localPairs, "frame_obj"));
		auto code = frame_obj.code();
		REQUIRE(code != nullptr);
		REQUIRE(code->name() == "<module>");
		REQUIRE(frame_obj.currentLineNumber() > 0);
		REQUIRE(frame_obj.globals() != nullptr);
		REQUIRE_NOTHROW(frame_obj.details());
	}
}
