#pragma once

#include "PyVarObject.h"
#include <string>

namespace PyExt::Remote {

	/// Represents a PyTypeObject in the debuggee's address space.
	class PYEXT_PUBLIC PyTypeObject : public PyVarObject
	{

	public: // Construction/Destruction.
		explicit PyTypeObject(Offset objectAddress);

	public: // Members.
		auto name() const -> std::string;
		auto documentation() const -> std::string;
		auto isPython2() const -> bool;
		auto repr(bool pretty = true) const -> std::string override;

	};

}