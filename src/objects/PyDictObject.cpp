#include "PyDictObject.h"

#include "../ExtHelpers.h"
#include "PyDictKeysObject.h"

#include <engextcpp.hpp>

#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <optional>
#include <utility>
using namespace std;


namespace {
	using PyExt::Remote::PyObject;
	using PyExt::Remote::PyDictKeysObject;

	auto getEntriesTable(ExtRemoteTyped dictObj) -> ExtRemoteTyped
	{
		// Python 2 stores a pointer to the entries table right in `ma_table`.
		if (dictObj.HasField("ma_table"))
			return dictObj.Field("ma_table");

		// Python 3 adds another layer of abstraction and stores a PyDictKeysObject in `ma_keys`.
		auto keys = dictObj.Field("ma_keys");
		auto dictKeysObj = make_unique<PyDictKeysObject>(keys.GetPtr());
		return dictKeysObj->getEntriesTable();
	}


	auto getEntriesTableSize(ExtRemoteTyped dictObj) -> PyObject::SSize
	{
		// Python 2.
		if (dictObj.HasField("ma_mask")) {
			// Mask is table size (a power of 2) minus 1.
			auto mask = dictObj.Field("ma_mask");
			return utils::readIntegral<PyObject::SSize>(mask) + 1;
		}

		// Python 3.
		auto keys = dictObj.Field("ma_keys");
		auto dictKeysObj = make_unique<PyDictKeysObject>(keys.GetPtr());
		return dictKeysObj->getEntriesTableSize();
	}


	auto getIsCombined(ExtRemoteTyped dictObj) -> bool
	{
		// Python 2 tables always act like Python 3 "combined" tables.
		if (dictObj.HasField("ma_mask")) {
			return true;
		}

		// Python 3.
		auto valuesPtr = dictObj.Field("ma_values").GetPtr();
		return (valuesPtr == 0);
	}

}

namespace PyExt::Remote {

	PyDict::~PyDict()
	{
	}


	auto PyDict::repr(bool pretty) const -> string
	{
		const auto elementSeparator = (pretty) ? "\n" : " "; //< Use a newline when pretty-print is on.
		const auto indentation = (pretty) ? "\t" : ""; //< Indent only when pretty is on.

		ostringstream oss;
		oss << '{' << elementSeparator;

		for (auto& pairValue : pairValues()) { //< TODO: Structured bindings. for (auto&& [key, value] : pairValues) {
			auto& key = pairValue.first;
			auto& value = pairValue.second;
			oss << indentation << key->repr(pretty) << ": " << value->repr(pretty) << ',' << elementSeparator;
		}

		oss << '}';
		return oss.str();
	}


	PyManagedDict::PyManagedDict(RemoteType::Offset keysPtr, RemoteType::Offset valuesPtr)
		: keysPtr(keysPtr), valuesPtr(valuesPtr)
	{
	}


	auto PyManagedDict::pairValues() const -> vector<pair<unique_ptr<PyObject>, unique_ptr<PyObject>>>
	{
		vector<pair<unique_ptr<PyObject>, unique_ptr<PyObject>>> pairs;

		auto keys = make_unique<PyDictKeysObject>(keysPtr);
		auto table = keys->getEntriesTable();
		auto tableSize = keys->getEntriesTableSize();
		auto nextValue = valuesPtr;
		auto ptrSize = utils::getPointerSize();

		for (auto i = 0; i < tableSize; ++i, nextValue += ptrSize) {
			auto dictEntry = table.ArrayElement(i);

			auto keyPtr = dictEntry.Field("me_key").GetPtr();
			auto valuePtr = ExtRemoteTyped("(PyObject**)@$extin", nextValue).Dereference().GetPtr();

			if (keyPtr == 0 || valuePtr == 0) //< The hash bucket might be empty.
				continue;

			auto key = PyObject::make(keyPtr);
			auto value = PyObject::make(valuePtr);
			pairs.push_back(make_pair(move(key), move(value)));
		}

		return pairs;
	}


	PyDictObject::PyDictObject(Offset objectAddress)
		: PyObject(objectAddress, "PyDictObject")
	{
	}


	auto PyDictObject::pairValues() const -> vector<pair<unique_ptr<PyObject>, unique_ptr<PyObject>>>
	{
		vector<pair<unique_ptr<PyObject>, unique_ptr<PyObject>>> pairs;

		auto table = getEntriesTable(remoteType());
		const auto tableSize = getEntriesTableSize(remoteType());
		const bool isCombined = getIsCombined(remoteType());

		optional<ExtRemoteTyped> valuesArray;
		if (!isCombined) {
			valuesArray = remoteType().Field("ma_values");
			utils::ignoreExtensionError([&] {
				// Python >= 3.11
				// Find full symbol name because there may be a name collision leading to truncated type info.
				valuesArray = ExtRemoteTyped(utils::getFullSymbolName("_dictvalues").c_str(), valuesArray->GetPtr(), true);
				valuesArray = valuesArray->Field("values");
			});				
		}

		for (SSize i = 0; i < tableSize; ++i) {
			auto dictEntry = table.ArrayElement(i);

			Offset keyPtr = dictEntry.Field("me_key").GetPtr();
			Offset valuePtr = 0;
			if (isCombined) {
				valuePtr = dictEntry.Field("me_value").GetPtr();
			} else {
				valuePtr = valuesArray->ArrayElement(i).GetPtr();
			}

			if (keyPtr == 0 || valuePtr == 0) //< The hash bucket might be empty.
				continue;

			auto key = PyObject::make(keyPtr);
			auto value = PyObject::make(valuePtr);
			pairs.push_back(make_pair(move(key), move(value)));
		}

		return pairs;
	}


	auto PyDictObject::repr(bool pretty) const -> string
	{
		return PyDict::repr(pretty);
	}

}
