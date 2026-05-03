#pragma once

#include <PyObject.h>
#include <PyStringValue.h>

#include <string>
#include <stdexcept>
#include <algorithm>

namespace TestHelpers {

	/// Finds the value in a dict pair-range whose key has the given string value.
	template <typename RangeT>
	auto findValueByKey(RangeT& pairRange, const std::string& key) -> PyExt::Remote::PyObject& {
		auto it = std::find_if(std::begin(pairRange), std::end(pairRange), [&](const auto& kv) {
			const auto* keyObj = dynamic_cast<PyExt::Remote::PyStringValue*>(kv.first.get());
			return keyObj != nullptr && keyObj->stringValue() == key;
		});

		if (it == std::end(pairRange))
			throw std::runtime_error("Value not found for key=" + key);
		return *it->second;
	}

}
