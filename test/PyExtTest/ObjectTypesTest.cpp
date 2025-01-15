#include "Catch.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"

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
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>
#include <vector>
#include <string>
#include <complex>
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


TEST_CASE("object_types.py has a stack frame with expected locals.", "[integration][object_types]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectTypesDumpFileNameOrDefault());

	// Set up pyext.dll so it thinks DbgEng is calling into it.
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto frames = dump.getMainThreadFrames();
	auto bottomFrame = frames.back();
	REQUIRE(bottomFrame->code()->name() == "<module>");

	auto locals = bottomFrame->locals();
	REQUIRE(locals != nullptr);

	auto localPairs = locals->pairValues();
	REQUIRE(localPairs.size() >= 17);


	SECTION("Value of string_obj.")
	{
		const string expectedValue = "TestString123";

		auto& string_obj = findValueByKey(localPairs, "string_obj");
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

		auto& big_string_obj = findValueByKey(localPairs, "big_string_obj");
		REQUIRE(big_string_obj.type().name() == "str");
		REQUIRE(big_string_obj.repr().find(expectedValue) != string::npos);
		REQUIRE(big_string_obj.details() == "");

		auto stringValue = dynamic_cast<PyStringValue&>(big_string_obj).stringValue();
		REQUIRE(stringValue == expectedValue);
	}


	SECTION("Value of bytes_obj.")
	{
		const string expectedValue = "TestBytes123";

		auto& bytes_obj = findValueByKey(localPairs, "bytes_obj");
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

		auto& byte_array_object = findValueByKey(localPairs, "byte_array_object");
		REQUIRE(byte_array_object.type().name() == "bytearray");
		REQUIRE(byte_array_object.repr() == "bytearray(b'"+ expectedValue + "')");
		REQUIRE(byte_array_object.details() == "");

		auto stringValue = dynamic_cast<PyStringValue&>(byte_array_object).stringValue();
		REQUIRE(stringValue == expectedValue);
	}


	SECTION("Value of int_obj.")
	{
		const int expectedValue = 1;

		auto& int_obj = findValueByKey(localPairs, "int_obj");
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

		auto& long_obj = dynamic_cast<PyLongObject&>(findValueByKey(localPairs, "long_obj"));
		const auto typeName = long_obj.type().name();
		REQUIRE((typeName == "int" || typeName == "long"));
		REQUIRE(long_obj.repr() == expectedValue);
		REQUIRE(long_obj.details() == "");
	}


	SECTION("Value of all_long_obj.")
	{
		const string expectedValue = "0,-123456,123456,987654321987654321987654321,-987654321987654321987654321";

		auto& tuple_obj = dynamic_cast<PyTupleObject&>(findValueByKey(localPairs, "all_long_obj"));
		auto value = tuple_obj.repr(false);
		std::regex chars_to_remove("[\\s()]+");
		value = std::regex_replace(value, chars_to_remove, "");
		REQUIRE(value == expectedValue);
	}


	SECTION("Value of float_obj.")
	{
		const double expectedValue = 3.1415;

		auto& float_obj = dynamic_cast<PyFloatObject&>(findValueByKey(localPairs, "float_obj"));
		REQUIRE(float_obj.type().name() == "float");
		REQUIRE(float_obj.repr().find(to_string(expectedValue)) == 0);
		REQUIRE(float_obj.details() == "");

		// Comparing floats can be error prone. If this fails, it might need some wiggle room in the compare.
		REQUIRE(float_obj.floatValue() == expectedValue);
	}


	SECTION("Value of complex_obj.")
	{
		const complex<double> expectedValue{1.5, -2.25};

		auto& complex_obj = dynamic_cast<PyComplexObject&>(findValueByKey(localPairs, "complex_obj"));
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
		auto& bool_true_obj = findValueByKey(localPairs, "bool_true_obj");
		REQUIRE(bool_true_obj.type().name() == "bool");


		const auto actualValue = bool_true_obj.repr();
		REQUIRE(actualValue == "True");
		REQUIRE(bool_true_obj.details() == "");
	}


	SECTION("Value of bool_false_obj.")
	{
		auto& bool_false_obj = findValueByKey(localPairs, "bool_false_obj");
		REQUIRE(bool_false_obj.type().name() == "bool");

		const auto actualValue = bool_false_obj.repr();
		REQUIRE(actualValue == "False");
		REQUIRE(bool_false_obj.details() == "");
	}


	SECTION("Value of none_obj.")
	{
		auto& none_obj = findValueByKey(localPairs, "none_obj");
		REQUIRE(none_obj.type().name() == "NoneType");
		REQUIRE(none_obj.repr() == "None");
		REQUIRE(none_obj.details() == "");
	}


	SECTION("Value of type_obj.")
	{
		auto& type_obj = dynamic_cast<PyTypeObject&>(findValueByKey(localPairs, "type_obj"));
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
		auto& not_implemented_obj = findValueByKey(localPairs, "not_implemented_obj");
		REQUIRE(not_implemented_obj.type().name() == "NotImplementedType");
		REQUIRE(not_implemented_obj.repr() == "NotImplemented");
		REQUIRE(not_implemented_obj.details() == "");
	}


	SECTION("Value of func_obj.")
	{
		auto& func_obj = dynamic_cast<PyFunctionObject&>(findValueByKey(localPairs, "func_obj"));
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
		auto& list_obj = dynamic_cast<PyListObject&>(findValueByKey(localPairs, "list_obj"));
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
		auto& tuple_obj = dynamic_cast<PyTupleObject&>(findValueByKey(localPairs, "tuple_obj"));
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
		auto& set_obj = dynamic_cast<PySetObject&>(findValueByKey(localPairs, "set_obj"));
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
		auto& dict_obj = dynamic_cast<PyDictObject&>(findValueByKey(localPairs, "dict_obj"));
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
}