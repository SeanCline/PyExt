#pragma once

#include "RemoteType.h"
#include "pyextpublic.h"

#include <memory>

class ExtRemoteTyped;

namespace PyExt::Remote {

	class PyFrameObject;

	/// Represents a PyInterpreterState instance in the debuggee's address space.
	/// @see https://github.com/python/cpython/blob/master/Include/pystate.h
	class PYEXT_PUBLIC PyThreadState : private RemoteType
	{
	public:
		explicit PyThreadState(const RemoteType& remoteType);
		~PyThreadState();

	public: // Members of the remote type.
		auto next() const -> std::unique_ptr<PyThreadState>;
		auto frame() const ->std::unique_ptr<PyFrameObject>;
		auto recursion_depth() const -> long;
		auto tracing() const -> long;
		auto use_tracing() const -> long;
		auto thread_id() const -> long;

	public: // Utility functions around the members.
		/// Returns a range of all the frames in this threadState.
		auto allFrames() const -> std::vector<PyFrameObject>; //< TODO: Return generator<PyFrameObject>
	};

}
