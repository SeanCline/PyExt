#pragma once

#include "PyVarObject.h"
#include <string>
#include <cstdint>

namespace PyExt::Remote {

	/// Represents a PyLongObject in the debuggee's address space.
	class PYEXT_PUBLIC PyLongObject : public PyVarObject
	{

	public: // Construction/Destruction.
		explicit PyLongObject(Offset objectAddress);

	public: // Members.
		auto isNegative() const -> bool;
		auto repr(bool pretty = true) const -> std::string override;

	};

}