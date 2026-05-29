#pragma once

#include "PyObject.h"

#include <memory>
#include <string>

namespace PyExt::Remote {

	class PyStringValue;
	class PyInterpreterFrame;
	class PyFrameObject;
	class PyFrame;

	/// Shared base for `generator`, `coroutine`, and `async_generator`.
	/// The three types differ only differ the prefix used on their fields ("gi", "cr", "ag") and by a display name.
	/// Their layouts embed a _PyInterpreterFrame.
	class PYEXT_PUBLIC PyGenObjectBase : public PyObject
	{

	public: // Members.
		auto name() const -> std::unique_ptr<PyStringValue>;
		auto qualname() const -> std::unique_ptr<PyStringValue>;
		auto frameState() const -> int;
		auto frameStateString() const -> std::string;
		auto frame() const -> std::unique_ptr<PyFrame>;
		auto repr(bool pretty = true) const -> std::string override;
		auto details() const -> std::string override;

	protected:
		explicit PyGenObjectBase(Offset objectAddress, const std::string& symbolName, const std::string& fieldPrefix, const std::string& displayName);

	private:
		std::string fieldPrefix_;
		std::string displayName_;

		auto prefixedField(const std::string& suffix) const -> std::string;
	};


	class PYEXT_PUBLIC PyGenObject : public PyGenObjectBase
	{
	public:
		explicit PyGenObject(Offset objectAddress);
	};


	class PYEXT_PUBLIC PyCoroObject : public PyGenObjectBase
	{
	public:
		explicit PyCoroObject(Offset objectAddress);
	};


	class PYEXT_PUBLIC PyAsyncGenObject : public PyGenObjectBase
	{
	public:
		explicit PyAsyncGenObject(Offset objectAddress);
	};

}
