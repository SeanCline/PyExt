#pragma once

#include <stdexcept>

namespace utils {

	/// Converts from one type to another. If equality is cannot preserved an overflow_error is raised.
	template <typename Target, typename Source>
	auto lossless_cast(Source value) -> Target
	{
		auto to = static_cast<Target>(value);

		if (static_cast<Source>(to) != value)
			throw std::overflow_error("lossless_cast could not represent value in target type.");

		return to;
	}

}