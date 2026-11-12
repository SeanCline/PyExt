#include "ReprBuilder.h"

#include "ExtHelpers.h"

#include <string>
#include <string_view>
using namespace std;

namespace utils {

	ReprBuilder::ReprBuilder(bool dml)
		: dml_(dml)
	{
	}


	auto ReprBuilder::text(string_view str) -> ReprBuilder&
	{
		if (dml_)
			out_ += escapeDml(str);
		else
			out_ += str;
		return *this;
	}


	auto ReprBuilder::markup(string_view str) -> ReprBuilder&
	{
		out_ += str;
		return *this;
	}


	auto ReprBuilder::link(string_view text, string_view command, string_view tooltip) -> ReprBuilder&
	{
		if (dml_)
			out_ += utils::link(text, command, tooltip);
		else
			out_ += text;
		return *this;
	}


	auto ReprBuilder::str() const -> string
	{
		return out_;
	}

}
