#pragma once

#include <stdexcept>
#include <utility>

namespace utils {

	/// Converts from one integer type to another. If the value cannot be preserved an overflow_error is raised.
	template <typename Target, typename Source>
	auto lossless_cast(Source value) -> Target
	{
		// A round trip through the target type isn't enough of a check on its own:
		// -1 survives a trip through an unsigned type despite not being representable in it.
		if (!std::in_range<Target>(value))
			throw std::overflow_error("lossless_cast could not represent value in target type.");

		return static_cast<Target>(value);
	}

}