#pragma once

#include "PyObject.h"
#include <cstdint>

namespace PyExt::Remote {

	/// Base class for all variable-sized objects.
	/// Represents a PyVarObject (objects with a dynamic allocation) in the debuggee's address space.
	class PYEXT_PUBLIC PyVarObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyVarObject(Offset objectAddress, const std::string& typeName = "PyVarObject");

	public: // Members.
		SSize size() const;

	};

}