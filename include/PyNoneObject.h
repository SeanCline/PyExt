#pragma once

#include "PyObject.h"
#include <string>

namespace PyExt::Remote {

	/// Represents an object of type NoneType. The most boring of the Python types.
	class PYEXT_PUBLIC PyNoneObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyNoneObject(Offset objectAddress);

	public: // Members.
		auto repr(bool pretty = true) const -> std::string override;

	};

}