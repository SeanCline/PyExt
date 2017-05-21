#pragma once

#include "PyObject.h"
#include <string>
#include <cstdint>

namespace PyExt::Remote {

	/// Represents a PyIntObject in the debuggee's address space.
	class PYEXT_PUBLIC PyIntObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyIntObject(Offset objectAddress);

	public: // Members.
		auto intValue() const -> std::int32_t;
		auto repr(bool pretty = true) const -> std::string override;

	};

}