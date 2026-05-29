#pragma once

#include "PyObject.h"
#include <memory>
#include <string>

namespace PyExt::Remote {

	/// Represents a Python `range` object in the debuggee's address space.
	class PYEXT_PUBLIC PyRangeObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyRangeObject(Offset objectAddress);

	public: // Members.
		auto start() const -> std::unique_ptr<PyObject>;
		auto stop() const -> std::unique_ptr<PyObject>;
		auto step() const -> std::unique_ptr<PyObject>;
		auto length() const -> std::unique_ptr<PyObject>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}
