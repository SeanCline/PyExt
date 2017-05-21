#pragma once

#include "PyObject.h"
#include <string>
#include <memory>

namespace PyExt::Remote {

	// Forward declarations.
	class PyCodeObject;
	class PyDictObject;
	class PyTupleObject;
	class PyDictObject;
	class PyListObject;
	class PyStringObject;

	/// Represents a PyFunctionObject in the debuggee's address space.
	class PYEXT_PUBLIC PyFunctionObject : public PyObject
	{

	public: // Construction/Destruction.
		explicit PyFunctionObject(Offset objectAddress);

	public: // Members.
		auto code() const -> std::unique_ptr<PyCodeObject>;
		auto globals() const -> std::unique_ptr<PyDictObject>;
		auto defaults() const -> std::unique_ptr<PyTupleObject>;
		auto kwdefaults() const -> std::unique_ptr<PyDictObject>;
		auto closure() const -> std::unique_ptr<PyTupleObject>;
		auto doc() const -> std::unique_ptr<PyObject>;
		auto name() const -> std::unique_ptr<PyStringObject>;
		auto dict() const -> std::unique_ptr<PyDictObject>;
		auto weakreflist() const -> std::unique_ptr<PyListObject>;
		auto module() const -> std::unique_ptr<PyObject>;
		auto annotations() const -> std::unique_ptr<PyDictObject>;
		auto qualname() const -> std::unique_ptr<PyStringObject>;
		auto repr(bool pretty = true) const -> std::string override;

	};

}