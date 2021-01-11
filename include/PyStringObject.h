#pragma once

#include "PyVarObject.h"
#include "PyStringValue.h"
#include <string>

namespace PyExt::Remote {

	/// Represents a PyStringObject or a PyBytesObject in the debuggee's address space.
	class PYEXT_PUBLIC PyBaseStringObject : public PyVarObject, public PyStringValue
	{

	public: // Construction/Destruction.
		explicit PyBaseStringObject(Offset objectAddress, const std::string& typeName);

	public: // Members.
		auto stringLength() const -> SSize;
		auto stringValue() const -> std::string override;
		virtual auto repr(bool pretty = true) const -> std::string override;

	};

	// Python3 has a PyBytesObject that takes the place of PyStringObject.
	// It shares the same layout as Python2's PyStringObject but has a different symbol name.

	/// Represents a Python3 PyBytesObject in the debuggee's address space.
	class PYEXT_PUBLIC PyBytesObject : public PyBaseStringObject {
	public:
		explicit PyBytesObject(Offset objectAddress);
		auto repr(bool pretty = true) const -> std::string override;
	};


	/// Represents a Python2 PyStringObject in the debuggee's address space.
	class PYEXT_PUBLIC PyStringObject : public PyBaseStringObject
	{
	public:
		explicit PyStringObject(Offset objectAddress);
	};

}