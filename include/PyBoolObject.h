#pragma once

#include "PyObject.h"
#include <string>
#include <cstdint>

namespace PyExt::Remote {

	/// Represents a PyBoolObject in the debuggee's address space.
	class PYEXT_PUBLIC PyBoolObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyBoolObject(Offset objectAddress);

	public: // Members.
		auto boolValue() const -> bool;
		auto repr(bool pretty = true) const -> std::string override;

	};

}