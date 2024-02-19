#pragma once

#include "PyFrame.h"
#include "PyVarObject.h"
#include <string>
#include <memory>

namespace PyExt::Remote {

	// Forward declarations.
	class PyDictObject;
	class PyCodeObject;
	class PyFunctionObject;

	/// Represents a PyFrameObject in the debuggee's address space.
	class PYEXT_PUBLIC PyFrameObject : public PyVarObject, public PyFrame
	{

	public: // Construction/Destruction.
		explicit PyFrameObject(Offset objectAddress);

	public: // Members.
		auto locals() const -> std::unique_ptr<PyDictObject> override;
		auto localsplus() const -> std::vector<std::pair<std::string, std::unique_ptr<PyObject>>> override;
		auto globals() const -> std::unique_ptr<PyDictObject> override;
		auto builtins() const -> std::unique_ptr<PyDictObject> override;
		auto code() const -> std::unique_ptr<PyCodeObject> override;
		auto previous() const->std::unique_ptr<PyFrame> override;
		auto back() const -> std::unique_ptr<PyFrameObject>;
		auto trace() const -> std::unique_ptr<PyFunctionObject>;
		auto lastInstruction() const -> int;
		auto currentLineNumber() const -> int override;
		auto repr(bool pretty = true) const -> std::string override;
		auto details() const -> std::string override;

	};

}