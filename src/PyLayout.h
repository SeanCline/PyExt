#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

class ExtRemoteTyped;

// Answers questions about how the debuggee's build of CPython lays out its data.
//
// CPython's internals change between versions, and between the default and free-threaded
// builds of the same version. Knowledge of those differences that isn't specific to one
// type of object is kept here, so that it's written down (and fixed) in one place.
//
// Wherever possible the answers come from the debuggee's symbols rather than from tables
// of what each version is known to do, since tables go stale with every release.
namespace PyExt::Layout {

	using Offset = std::uint64_t;

	/// Forgets everything remembered about the debuggee's symbols.
	/// Must be called whenever the debugger may have moved on to a different target or had its symbols
	/// reloaded, which in practice means at the start of every command.
	auto resetCache() -> void;

	/// Returns a pointer to an instance of one of the Python module's types at `address`.
	/// Equivalent to `ExtRemoteTyped(typeName, address, true)`, without searching for the type every time.
	auto typedPointer(std::string_view typeName, Offset address) -> ExtRemoteTyped;

	/// Size of a pointer in the debuggee, in bytes.
	auto pointerSize() -> int;

	/// Name of the module that holds the Python runtime, such as "python315" or "python315t".
	/// Empty if there is no such module.
	auto pythonModuleName() -> std::string;

	/// Qualifies a symbol with the Python module's name. An unqualified name makes the debugger search
	/// every module in the process, which is slow and can find a different module's type of the same name.
	auto qualifiedSymbolName(std::string_view symbolName) -> std::string;

	/// True for builds without a GIL (the "t" builds of Python 3.13+), which use a different object header.
	auto isFreeThreaded() -> bool;

	/// Looks up the name CPython's own source gives to a value of one of its enums, such as "FRAME_SUSPENDED".
	/// Returns nullopt if the enum isn't in the symbols or has no enumerator with that value.
	auto enumeratorName(std::string_view enumName, std::int64_t value) -> std::optional<std::string>;

	/// Recovers the object address from the bits of a `_PyStackRef`, or returns 0 if it doesn't refer to an object.
	/// Also accepts the plain `PyObject*` that versions without stack references store in the same places.
	auto stackRefToAddress(Offset bits) -> Offset;

	/// Offset from the start of an object to the pointer-sized slot before it that holds its managed `__dict__`.
	/// Only meaningful for objects whose type has Py_TPFLAGS_MANAGED_DICT.
	auto managedDictSlotOffset() -> std::int64_t;

}
