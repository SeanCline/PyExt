#pragma once

#include "PyObject.h"

#include <memory>
#include <string>
#include <vector>

namespace PyExt::Remote {

	/// Represents a PySetObject in the debuggee's address space.
	class PYEXT_PUBLIC PySetObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PySetObject(Offset objectAddress);

	public: // Members.
		auto numItems() const -> SSize;
		auto listValue() const -> std::vector<std::unique_ptr<PyObject>>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}