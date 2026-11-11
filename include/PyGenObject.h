#pragma once

#include "PyObject.h"

#include <memory>
#include <optional>
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
		/// The raw `gi_frame_state` value, whose meaning depends on the Python version. nullopt before Python 3.11.
		auto frameState() const -> std::optional<int>;
		/// A lowercase name for the state such as "suspended", or empty if the state isn't available.
		auto frameStateString() const -> std::string;
		auto frame() const -> std::unique_ptr<PyFrame>;
		auto repr(bool pretty = true) const -> std::string override;
		auto details() const -> std::string override;

	protected:
		explicit PyGenObjectBase(Offset objectAddress, const std::string& symbolName, const std::string& fieldPrefix, const std::string& displayName);

	private:
#pragma warning (push)
#pragma warning (disable: 4251) //< Hide warnings about exporting private symbols.
		std::string fieldPrefix_;
		std::string displayName_;
#pragma warning (pop)

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
