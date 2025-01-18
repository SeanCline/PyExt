#pragma once

#include "PyVarObject.h"
#include "PyMemberDef.h"
#include "PyTupleObject.h"
#include "PyDictObject.h"
#include <array>
#include <string>

namespace PyExt::Remote {

	/// Represents a PyTypeObject in the debuggee's address space.
	class PYEXT_PUBLIC PyTypeObject : public PyVarObject
	{
	public: // Statics.
		static auto builtinTypes() -> const std::vector<std::string>&;

	public: // Construction/Destruction.
		explicit PyTypeObject(Offset objectAddress);

	public: // Members.
		auto name() const -> std::string;
		auto basicSize() const -> SSize;
		auto itemSize() const -> SSize;
		auto documentation() const -> std::string;
		auto members() const -> std::vector<std::unique_ptr<PyMemberDef>>;
		auto isManagedDict() const -> bool;
		auto getStaticBuiltinIndex() const -> SSize;
		auto hasInlineValues() const -> bool;
		auto dictOffset() const -> SSize;
		auto mro() const -> std::unique_ptr<PyTupleObject>;
		auto isPython2() const -> bool;
		auto dict() const -> std::unique_ptr<PyDict> override;
		auto repr(bool pretty = true) const -> std::string override;
		
	};

}