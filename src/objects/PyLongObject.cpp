#include "PyLongObject.h"

#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>
using namespace std;

namespace PyExt::Remote {

	PyLongObject::PyLongObject(Offset objectAddress, const bool isBool)
		: PyVarObject(objectAddress, "PyLongObject"), isBool(isBool)
	{
	}


	auto PyLongObject::isNegative() const -> bool
	{
		return size() < 0;
	}


	auto PyLongObject::repr(bool /*pretty*/) const -> string
	{
		auto digits = remoteType().Field("ob_digit");
		if (isBool) {
			auto digit = digits.ArrayElement(0);
			const auto value = utils::readIntegral<uint64_t>(digit);
			return value == 1 ? "True"s : "False"s;
		}

		const auto bytesPerDigit = digits.ArrayElement(0).GetTypeSize();

		// Set up our constants based on how CPython was compiled.
		// See: https://github.com/python/cpython/blob/master/Include/longintrepr.h
		int64_t SHIFT = 0, DECIMAL_BASE = 0, DECIMAL_SHIFT = 0;
		if (bytesPerDigit == 4) {
			SHIFT = 30;
			DECIMAL_SHIFT = 9;
			DECIMAL_BASE = 1000000000;
		} else if (bytesPerDigit == 2) {
			SHIFT = 15;
			DECIMAL_SHIFT = 4;
			DECIMAL_BASE = 10000;
		} else {
			throw runtime_error("Unexpected PyLong bytes per digit of " + to_string(bytesPerDigit));
		}
		//const auto BASE = static_cast<int64_t>(1) << SHIFT;
		//const auto MASK = BASE - 1;

		// Convert from BASE to DECIMAL_BASE and store the result in `buff`.
		vector<uint64_t> buff;
		const auto numDigits = abs(size());
		for (int64_t i = numDigits - 1; i >= 0; --i) {
			auto hiElement = digits.ArrayElement(i);
			auto hi = utils::readIntegral<uint64_t>(hiElement);
			for (auto& buffDigit : buff) {
				uint64_t z = buffDigit << SHIFT | hi;
				hi = z / DECIMAL_BASE;
				buffDigit = z - hi * DECIMAL_BASE;
			}

			while (hi) {
				buff.push_back(hi % DECIMAL_BASE);
				hi /= DECIMAL_BASE;
			}
		}

		// If the buffer is empty, use 0.
		if (buff.empty())
			buff.push_back(0);

		// Convert `buff`  from DECIMAL_BASE to base 10 and store the characters in `out`.
		string out;
		for (size_t i = 0; i < buff.size() - 1; i++) {
			auto rem = buff[i];
			for (int64_t j = 0; j < DECIMAL_SHIFT; ++j) {
				out.push_back('0' + rem % 10);
				rem /= 10;
			}
		}

		// Consume the last digit.
		auto rem = buff.back();
		do {
			out.push_back('0' + rem % 10);
			rem /= 10;
		} while (rem != 0);

		// Append a negative sign if needed.
		if (isNegative())
			out.push_back('-');

		reverse(out.begin(), out.end());
		return out;
	}

}
