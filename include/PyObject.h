#pragma once

#include "pyextpublic.h"

#include <cstdint>
#include <string>
#include <memory>


class ExtRemoteTyped;

namespace PyExt::Remote {

	class PyTypeObject; //< Forward Declaration.

	/// Represents a PyObject in the debuggee's address space. Base class for all types of PyObject.
	class PYEXT_PUBLIC PyObject
	{

	public: // Typedefs.
		using Offset = std::uint64_t;
		using SSize = std::int64_t;

	public: // Construction/Destruction.
		explicit PyObject(Offset objectAddress, const std::string& symbolName = "PyObject");
		virtual ~PyObject();

		/// Polymorphic constructor. Creates the most-derived PyObject it can.
		static auto make(PyObject::Offset remoteAddress) -> std::unique_ptr<PyObject>;

	public: // Copy/Move.
		PyObject(const PyObject&);
		PyObject& operator=(const PyObject&);
		PyObject(PyObject&&);
		PyObject& operator=(PyObject&&);

	public: // Members.
		auto offset() const -> Offset;
		auto symbolName() const -> std::string;
		auto refCount() const -> SSize;
		auto type() const -> PyTypeObject;
		virtual auto repr(bool pretty = true) const -> std::string;

	protected: // Helpers for more derived classes.
		/// Access to the PyObject's memory in the debuggee.
		auto remoteObj() const -> ExtRemoteTyped&;

		/// Returns a field by name in the `ob_base` member.
		auto baseField(const std::string& fieldName) const -> ExtRemoteTyped;

	private: // Data.
#pragma warning (push)
#pragma warning (disable: 4251) //< Hide warnings about exporting private symbols.
		std::shared_ptr<ExtRemoteTyped> remoteObj_;
		std::string symbolName_;
#pragma warning (pop)

	};

}