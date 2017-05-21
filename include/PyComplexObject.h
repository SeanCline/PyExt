#pragma once

#include "PyObject.h"
#include <string>
#include <complex>
#include <cstdint>

namespace PyExt::Remote {

	/// Represents a PyComplexObject in the debuggee's address space.
	class PYEXT_PUBLIC PyComplexObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyComplexObject(Offset objectAddress);

	public: // Members.
		auto complexValue() const -> std::complex<double>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}