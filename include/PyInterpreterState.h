#pragma once

#include "RemoteType.h"
#include "pyextpublic.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <optional>

class ExtRemoteTyped;

namespace PyExt::Remote {

	class PyDictObject;
	class PyThreadState;

	/// Represents a PyInterpreterState instance in the debuggee's address space.
	/// @see https://github.com/python/cpython/blob/master/Include/pystate.h
	class PYEXT_PUBLIC PyInterpreterState : private RemoteType
	{
	public: // Contruction/Destruction.
		explicit PyInterpreterState(Offset objectAddress);
		~PyInterpreterState();

		/// Returns the Python's global interpreter state instance.
		static auto makeAutoInterpreterState() -> std::unique_ptr<PyInterpreterState>;

		/// Returns a range of all interpreter states in the process, starting with the autoInterpreterState.
		static auto allInterpreterStates() -> std::vector<PyInterpreterState>; //< TODO: Return generator<PyInterpreterState>

		/// Returns the PyThreadState associated with a thread id or None if no such thread exists.
		static auto findThreadStateBySystemThreadId(std::uint64_t systemThreadId) -> std::optional<PyThreadState>;

	public: // Members of the remote type.
		auto next() const -> std::unique_ptr<PyInterpreterState>;
		auto tstate_head() const -> std::unique_ptr<PyThreadState>;
		auto modules() const -> std::unique_ptr<PyDictObject>;
		auto sysdict() const -> std::unique_ptr<PyDictObject>;
		auto builtins() const -> std::unique_ptr<PyDictObject>;

	public: // Utility functions around the members.
		/// Returns a range of all the threads in this interpreter.
		auto allThreadStates() const -> std::vector<PyThreadState>; //< TODO: Return generator<PyThreadState>

	};

}
