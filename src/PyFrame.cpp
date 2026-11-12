#include "PyFrame.h"

#include "PyObject.h"
#include "ReprBuilder.h"

#include <string>
using namespace std;

namespace PyExt::Remote {

	PyFrame::~PyFrame()
	{
	}


	auto PyFrame::details() const -> string
	{
		utils::ReprBuilder out(true); //< Details are always printed as DML.
		out.text("localsplus: {\n");

		for (auto const& [name, value] : localsplus()) {
			// The names come from PyCodeObject as reprs, so they're already markup.
			if (value != nullptr)
				out.text("\t").markup(name).text(": ").markup(value->repr(true)).text(",\n");
		}

		out.text("}");
		return out.str();
	}
}
