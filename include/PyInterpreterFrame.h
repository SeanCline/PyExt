#pragma once

#include "PyFrame.h"
#include "RemoteType.h"
#include "pyextpublic.h"

#include <memory>
#include <optional>

class ExtRemoteTyped;

namespace PyExt::Remote {

	class PyDictObject;
	class PyCodeObject;
	class PyFrameObject;
	class PyFunctionObject;
	class PyObject;

	/// Python 3.11 and later
	/// @see https://github.com/python/cpython/blob/master/include/internal/pycore_frame.h
	class PYEXT_PUBLIC PyInterpreterFrame : public RemoteType, public PyFrame
	{
	public:
		// tlbcIndex identifies the owning thread's slot in co_tlbc on the
		// free-threaded build. interpStateOffset is the address of the
		// owning PyInterpreterState — needed under FT to resolve
		// _PyStackRef indices through its stackref table. Frames walked
		// from PyThreadState seed both; standalone constructions
		// (!pyinterpreterframe, embedded generator frame) leave them
		// nullopt and the FT lookups fall back gracefully (entries[0] for
		// bytecode, no-op for stackref decoding).
		explicit PyInterpreterFrame(const RemoteType& remoteType,
			std::optional<int> tlbcIndex = std::nullopt,
			std::optional<std::uint64_t> interpStateOffset = std::nullopt);

	public: // Members of the remote type.
		using RemoteType::offset;
		auto locals() const->std::unique_ptr<PyDictObject> override;
		auto localsplus() const->std::vector<std::pair<std::string, std::unique_ptr<PyObject>>> override;
		auto globals() const->std::unique_ptr<PyDictObject> override;
		auto code() const->std::unique_ptr<PyCodeObject> override;
		auto previous() const->std::unique_ptr<PyFrame> override;

		// Address of the currently-executing instruction: `instr_ptr` (3.13+) or `prev_instr` (<3.12).
		auto prevInstruction() const -> RemoteType::Offset;

		auto currentLineNumber() const -> int override;
		auto isIncomplete() const -> bool;

	private:
#pragma warning (push)
#pragma warning (disable: 4251) //< Hide warnings about exporting private symbols.
		std::optional<int> tlbcIndex_;
		std::optional<std::uint64_t> interpStateOffset_;
#pragma warning (pop)
	};

}
