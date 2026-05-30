#pragma once

#include "RemoteType.h"
#include "pyextpublic.h"

#include <memory>
#include <optional>

class ExtRemoteTyped;

namespace PyExt::Remote {

	class PyFrame;
	class PyFrameObject;
	class PyInterpreterFrame;

	/// Represents a PyInterpreterState instance in the debuggee's address space.
	/// @see https://github.com/python/cpython/blob/master/Include/pystate.h
	class PYEXT_PUBLIC PyThreadState : private RemoteType
	{
	public:
		explicit PyThreadState(const RemoteType& remoteType);
		~PyThreadState();

	public: // Members of the remote type.
		auto next() const -> std::unique_ptr<PyThreadState>;
		auto currentFrame() const -> std::unique_ptr<PyFrame>;
		auto tracing() const -> long;
		auto thread_id() const -> long;
		// Index into PyCodeObject::co_tlbc on the free-threaded build; nullopt
		// on GIL builds and pre-3.13 free-threaded prototypes that lack the field.
		auto tlbcIndex() const -> std::optional<int>;
		// Address of the owning PyInterpreterState (the `interp` back-pointer).
		// Returns nullopt only if the field is missing or null in the dump.
		auto interpreterStateOffset() const -> std::optional<std::uint64_t>;

	public: // Utility functions around the members.
		/// Returns a range of all the frames in this threadState.
		auto allFrames() const -> std::vector<std::shared_ptr<PyFrame>>; //< TODO: Return generator<PyFrame>
	};

}
