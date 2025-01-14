#pragma once

#include "PyObject.h"

#include <string>
#include <cstdint>

namespace PyExt::Remote {

	/// Represents a PyLongObject in the debuggee's address space.
	class PYEXT_PUBLIC PyLongObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyLongObject(Offset objectAddress, const bool isBool = false);

	public: // Members.
		auto repr(bool pretty = true) const -> std::string override;

	private:
		const bool isBool;

	};

}