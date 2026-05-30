#pragma once

#include "pyextpublic.h"
#include "RemoteType.h"

#include <cstdint>
#include <string>
#include <memory>


class ExtRemoteTyped;

namespace PyExt::Remote {

	class PyTypeObject; //< Forward Declaration.
	class PyDict; //< Forward Declaration.

	/// Represents a PyObject in the debuggee's address space. Base class for all types of PyObject.
	class PYEXT_PUBLIC PyObject : private RemoteType
	{

	public: // Typedefs.
		using RemoteType::Offset;
		using RemoteType::SSize;

	public: // Construction/Destruction.
		explicit PyObject(Offset objectAddress, const std::string& symbolName = "PyObject");
		virtual ~PyObject();

		// Polymorphic constructor. Creates the most-derived PyObject it can.
		static auto make(PyObject::Offset remoteAddress) -> std::unique_ptr<PyObject>;
		// Constructor by type name. Necessary to get the base type repr for types derived from built-in types.
		static auto make(PyObject::Offset remoteAddress, const std::string& typeName) -> std::unique_ptr<PyObject>;
		using RemoteType::readOffsetArray;

		/// Size of `struct _object` for the loaded build. Doubles on the
		/// free-threaded build, so callers should use this rather than the
		/// hard-coded `2 * pointerSize`.
		static auto headerSize() -> SSize;

		/// Byte offset (negative) from a managed-dict object's `offset()` to
		/// its `PyManagedDictPointer` slot in the pre-header. Shifts from
		/// `-3 * pointerSize` (GIL build, Python 3.13+) to `-1 * pointerSize`
		/// (free-threaded build).
		static auto managedDictOffset() -> SSize;

	public: // Members.
		using RemoteType::offset;
		using RemoteType::symbolName;
		auto refCount() const -> SSize;
		auto isImmortal() const -> bool;
		auto type() const -> PyTypeObject;
		auto slots() const -> std::vector<std::pair<std::string, std::unique_ptr<PyObject>>>;
		auto managedDict() const -> std::unique_ptr<PyDict>;
		virtual auto dict() const -> std::unique_ptr<PyDict>;
		virtual auto repr(bool pretty = true) const -> std::string;
		virtual auto details() const -> std::string;

	protected: // Helpers for more derived classes.
		/// Returns a field by name in the `ob_base` member.
		auto baseField(const std::string& fieldName) const -> ExtRemoteTyped;
		using RemoteType::remoteType;

	};

}