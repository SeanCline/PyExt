#pragma once

#include "PyVarObject.h"
#include "PyMemberDef.h"
#include "PyTupleObject.h"
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
		auto members() const -> std::vector<std::unique_ptr<PyMemberDef>>;
		auto dictOffset() const -> SSize;
		auto mro() const -> std::unique_ptr<PyTupleObject>;
		auto isPython2() const -> bool;
		auto repr(bool pretty = true) const -> std::string override;

	};

}