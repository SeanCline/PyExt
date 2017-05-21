#pragma once

#include "PyVarObject.h"
#include "PyStringValue.h"
#include <string>
#include <vector>

namespace PyExt::Remote {

	/// Represents a PyByteArrayObject in the debuggee's address space.
	class PYEXT_PUBLIC PyByteArrayObject : public PyVarObject, public PyStringValue
	{

	public: // Construction/Destruction.
		explicit PyByteArrayObject(Offset objectAddress);

	public: // Members.
		auto arrayValue() const -> std::vector<uint8_t>;
		auto stringLength() const -> SSize;
		auto stringValue() const -> std::string override;
		auto repr(bool pretty = true) const -> std::string override;

	};

}