#pragma once

#include "PyObject.h"

#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace PyExt::Remote {

	/// Represents a PyDictObject in the debuggee's address space.
	class PYEXT_PUBLIC PyDictObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyDictObject(Offset objectAddress);

	public: // Members.
		auto pairValues() const -> std::vector<std::pair<std::unique_ptr<PyObject>, std::unique_ptr<PyObject>>>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}