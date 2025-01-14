#pragma once

#include "pyextpublic.h"

namespace PyExt::Remote {

	class Offset;
	class PyDictObject;
	class PyCodeObject;
	class PyFunctionObject;
	class PyObject;

	/// Common interface for PyFrameObject and PyInterpreterFrame
	class PYEXT_PUBLIC PyFrame
	{
	public:
		virtual ~PyFrame();

	public: // Members of the remote type.
		virtual auto locals() const -> std::unique_ptr<PyDictObject> = 0;
		virtual auto localsplus() const -> std::vector<std::pair<std::string, std::unique_ptr<PyObject>>> = 0;
		virtual auto globals() const -> std::unique_ptr<PyDictObject> = 0;
		virtual auto code() const -> std::unique_ptr<PyCodeObject> = 0;
		virtual auto previous() const -> std::unique_ptr<PyFrame> = 0;
		virtual auto currentLineNumber() const -> int = 0;
		virtual auto details() const -> std::string;
	};

}
