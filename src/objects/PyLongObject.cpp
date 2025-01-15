#include "PyLongObject.h"

#include "PyVarObject.h"

#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
#include <stdexcept>
#include <vector>
#include <algorithm>
using namespace std;

namespace PyExt::Remote {

	PyLongObject::PyLongObject(Offset objectAddress, const bool isBool)
		: PyObject(objectAddress, "PyLongObject"), isBool(isBool)
	{
	}


	auto PyLongObject::repr(bool /*pretty*/) const -> string
	{
		ExtRemoteTyped digits;
		SSize digitCount;
		SSize sign;
		auto isCompact = false;

		if (remoteType().HasField("long_value")) {
			// Python 3.12+
			auto longValue = remoteType().Field("long_value");
			digits = longValue.Field("ob_digit");

			auto tagRaw = longValue.Field("lv_tag");
			auto tag = utils::readIntegral<uint64_t>(tagRaw);
			digitCount = tag >> 3;  // _PyLong_NON_SIZE_BITS
			sign = 1 - (tag & 3);  // _PyLong_SIGN_MASK
			isCompact = tag < (2 << 3);  // _PyLong_NON_SIZE_BITS
		} else {
			digits = remoteType().Field("ob_digit");
			auto varObject = PyVarObject(offset());
			sign = varObject.size();
			digitCount = abs(sign);
		}

		auto firstDigit = digits.ArrayElement(0);
		auto firstDigitValue = utils::readIntegral<uint64_t>(firstDigit);

		if (isBool)
			return firstDigitValue == 1 ? "True"s : "False"s;

		if (isCompact)
			return to_string(sign * (SSize)firstDigitValue);

		const auto bytesPerDigit = firstDigit.GetTypeSize();

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
		for (int64_t i = digitCount - 1; i >= 0; --i) {
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
		if (sign < 0)
			out.push_back('-');

		reverse(out.begin(), out.end());
		return out;
	}

}
