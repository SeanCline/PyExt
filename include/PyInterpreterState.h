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
		explicit PyInterpreterState(const RemoteType& remoteType);
		~PyInterpreterState();
		
		/// Returns the Python's global interpreter state instance.
		static auto makeAutoInterpreterState() -> std::unique_ptr<PyInterpreterState>;

		/// Returns a range of all interpreter states in the process, starting with the autoInterpreterState.
		static auto allInterpreterStates() -> std::vector<PyInterpreterState>; //< TODO: Return generator<PyInterpreterState>

		/// Returns the PyThreadState associated with a thread id or None if no such thread exists.
		static auto findThreadStateBySystemThreadId(std::uint64_t systemThreadId) -> std::optional<PyThreadState>;

		/// Provide a way to manually specify the interpreter state used by makeAutoInterpreterState().
		static void setAutoInterpreterStateExpression(const std::string& expression);


	public: // Members of the remote type.
		auto next() const -> std::unique_ptr<PyInterpreterState>;
		auto tstate_head() const -> std::unique_ptr<PyThreadState>;

	public: // Utility functions around the members.
		/// Returns a range of all the threads in this interpreter.
		auto allThreadStates() const -> std::vector<PyThreadState>; //< TODO: Return generator<PyThreadState>

	private:
#pragma warning (push)
#pragma warning (disable: 4251) //< Hide warnings about exporting private symbols.
		static std::string autoInterpreterStateExpressionOverride;
#pragma warning (pop)
	};

}
