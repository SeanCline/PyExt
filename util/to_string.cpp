#include "to_string.h"

#include <engextcpp.hpp>

std::string to_string(const ExtBuffer<char>& b)
{
	if (b.GetRawBuffer() == nullptr || b.GetEltsUsed() == 0 || *b.GetBuffer() == '\0')
		return {};

	return { b.GetBuffer(), b.GetEltsUsed() - 1 };
}