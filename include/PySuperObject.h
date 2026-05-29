#pragma once

#include "PyObject.h"
#include <memory>
#include <string>

namespace PyExt::Remote {

	class PyTypeObject;

	/// Represents a Python `super` object in the debuggee's address space.
	class PYEXT_PUBLIC PySuperObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PySuperObject(Offset objectAddress);

	public: // Members.
		auto thisClass() const -> std::unique_ptr<PyTypeObject>;
		auto thisObject() const -> std::unique_ptr<PyObject>;
		auto objectType() const -> std::unique_ptr<PyTypeObject>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}
