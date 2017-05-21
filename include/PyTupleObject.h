#pragma once

#include "PyVarObject.h"

#include <memory>
#include <string>
#include <vector>

namespace PyExt::Remote {

	/// Represents a PyTupleObject in the debuggee's address space.
	class PYEXT_PUBLIC PyTupleObject : public PyVarObject
	{

	public: // Construction/Destruction.
		explicit PyTupleObject(Offset objectAddress);

	public: // Members.
		auto numItems() const -> SSize;
		auto at(SSize index) const -> std::unique_ptr<PyObject>;
		auto listValue() const -> std::vector<std::unique_ptr<PyObject>>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}