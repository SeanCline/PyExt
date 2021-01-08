#pragma once

#include "PyObject.h"
#include <string>

namespace PyExt::Remote {

	/// Represents a PyBoolObject in the debuggee's address space.
	class PYEXT_PUBLIC PyCellObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyCellObject(Offset objectAddress);

	public: // Members.
		auto objectReference() const -> std::unique_ptr<PyObject>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}