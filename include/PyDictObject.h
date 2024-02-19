#pragma once

#include "PyObject.h"

#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace PyExt::Remote {

	class PyDictKeysObject;

	/// Common interface for PyDictObject and PyManagedDict
	class PYEXT_PUBLIC PyDict
	{

	public: // Construction/Destruction.
		virtual ~PyDict();

	public: // Members.
		virtual auto pairValues() const->std::vector<std::pair<std::unique_ptr<PyObject>, std::unique_ptr<PyObject>>> = 0;
		virtual auto repr(bool pretty = true) const->std::string;

	};


	class PYEXT_PUBLIC PyManagedDict : public PyDict
	{

	public: // Construction/Destruction.
		explicit PyManagedDict(RemoteType::Offset keysPtr, RemoteType::Offset valuesPtrPtr);

	public: // Members.
		auto pairValues() const->std::vector<std::pair<std::unique_ptr<PyObject>, std::unique_ptr<PyObject>>> override;

	private:
		RemoteType::Offset keysPtr;
		RemoteType::Offset valuesPtrPtr;

	};


	/// Represents a PyDictObject in the debuggee's address space.
	class PYEXT_PUBLIC PyDictObject : public PyObject, public PyDict
	{

	public: // Construction/Destruction.
		explicit PyDictObject(Offset objectAddress);

	public: // Members.
		auto pairValues() const -> std::vector<std::pair<std::unique_ptr<PyObject>, std::unique_ptr<PyObject>>> override;
		auto repr(bool pretty = true) const -> std::string override;

	};

}