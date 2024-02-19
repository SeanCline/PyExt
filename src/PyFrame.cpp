#include "PyFrame.h"

#include "PyObject.h"

#include <string>
#include <sstream>
using namespace std;

namespace PyExt::Remote {

	PyFrame::~PyFrame()
	{
	}


	auto PyFrame::details() const -> string
	{
		const auto elementSeparator = "\n";
		const auto indentation = "\t";

		ostringstream oss;
		oss << "localsplus: {" << elementSeparator;

		for (auto const& pairValue : localsplus()) {
			auto const& key = pairValue.first;
			auto const& value = pairValue.second;
			if (value != nullptr)
				oss << indentation << key << ": " << value->repr(true) << ',' << elementSeparator;
		}

		oss << '}';
		return oss.str();
	}
}
