#pragma once

#include "PyObject.h"
#include <string>
#include <cstdint>

namespace PyExt::Remote {

	/// Represents a PyFloatObject in the debuggee's address space.
	class PYEXT_PUBLIC PyFloatObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyFloatObject(Offset objectAddress);

	public: // Members.
		auto floatValue() const -> double;
		auto repr(bool pretty = true) const -> std::string override;

	};

}