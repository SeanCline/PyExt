#pragma once

#include "PyVarObject.h"
#include <memory>
#include <string>
#include <vector>

namespace PyExt::Remote {

	/// Represents a PyMemoryViewObject in the debuggee's address space.
	class PYEXT_PUBLIC PyMemoryViewObject : public PyVarObject
	{

	public: // Construction/Destruction.
		explicit PyMemoryViewObject(Offset objectAddress);

	public: // Members.
		// The exporter of the underlying buffer (often bytes, bytearray, or array).
		auto exporter() const -> std::unique_ptr<PyObject>;
		auto length() const -> SSize;
		auto itemsize() const -> SSize;
		auto readonly() const -> bool;
		auto ndim() const -> int;
		auto format() const -> std::string;
		auto shape() const -> std::vector<SSize>;
		auto strides() const -> std::vector<SSize>;
		auto repr(bool pretty = true) const -> std::string override;
		auto details() const -> std::string override;

	};

}
