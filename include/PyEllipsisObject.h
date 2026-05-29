#pragma once

#include "PyObject.h"
#include <string>

namespace PyExt::Remote {

	/// Represents the Ellipsis singleton in the debuggee's address space.
	class PYEXT_PUBLIC PyEllipsisObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyEllipsisObject(Offset objectAddress);

	public: // Members.
		auto repr(bool pretty = true) const -> std::string override;

	};

}
