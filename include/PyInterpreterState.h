#pragma once

#include "RemoteType.h"
#include "pyextpublic.h"

#include <memory>

class ExtRemoteTyped;

namespace PyExt::Remote {

	class PyDictObject;
	class PyThreadState;

	/// Represents a PyInterpreterState instance in the debuggee's address space.
	/// @see https://github.com/python/cpython/blob/master/Include/pystate.h
	class PYEXT_PUBLIC PyInterpreterState : private RemoteType
	{
	public:
		explicit PyInterpreterState(Offset objectAddress);
		~PyInterpreterState();

	public:
		/// Returns the Python's global interpreter state instance.
		static auto makeAutoInterpreterState() -> std::unique_ptr<PyInterpreterState>;

	public: // Members of the remote type.
		auto next() const -> std::unique_ptr<PyInterpreterState>;
		auto tstate_head() const -> std::unique_ptr<PyThreadState>;
		auto modules() const -> std::unique_ptr<PyDictObject>;
		auto sysdict() const -> std::unique_ptr<PyDictObject>;
		auto builtins() const -> std::unique_ptr<PyDictObject>;

	};

}
