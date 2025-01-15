#pragma once

#include "PyFrame.h"
#include "RemoteType.h"
#include "pyextpublic.h"

#include <memory>

class ExtRemoteTyped;

namespace PyExt::Remote {

	class PyDictObject;
	class PyCodeObject;
	class PyFrameObject;
	class PyFunctionObject;
	class PyObject;

	/// Python 3.11 and later
	/// @see https://github.com/python/cpython/blob/master/include/internal/pycore_frame.h
	class PYEXT_PUBLIC PyInterpreterFrame : public RemoteType, public PyFrame
	{
	public:
		explicit PyInterpreterFrame(const RemoteType& remoteType);

	public: // Members of the remote type.
		using RemoteType::offset;
		auto locals() const->std::unique_ptr<PyDictObject> override;
		auto localsplus() const->std::vector<std::pair<std::string, std::unique_ptr<PyObject>>> override;
		auto globals() const->std::unique_ptr<PyDictObject> override;
		auto code() const->std::unique_ptr<PyCodeObject> override;
		auto previous() const->std::unique_ptr<PyFrame> override;
		auto prevInstruction() const -> int;
		auto currentLineNumber() const -> int override;
	};

}
