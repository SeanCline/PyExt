#include "extension.h"

#include "PyFrameObject.h"
#include "PyInterpreterFrame.h"
#include "PyDictObject.h"
#include "PyCodeObject.h"
#include "PyInterpreterState.h"
#include "PyThreadState.h"
#include "PyTypeObject.h"
using namespace PyExt::Remote;

#include "ExtHelpers.h"
#include "ReprBuilder.h"

#include <engextcpp.hpp>

#include <string>
#include <utility>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <optional>
using namespace std;


namespace {

	auto getCurrentThreadId(IDebugSystemObjects* pSystem) -> uint64_t
	{
		ULONG currentThreadSystemId = 0;
		HRESULT hr = pSystem->GetCurrentThreadId(&currentThreadSystemId);
		if (FAILED(hr))
			throw runtime_error("Failed to retreive current thread id.");
		return currentThreadSystemId;
	}


	auto getCurrentThreadSystemId(IDebugSystemObjects* pSystem) -> uint64_t
	{
		ULONG currentThreadSystemId = 0;
		HRESULT hr = pSystem->GetCurrentThreadSystemId(&currentThreadSystemId);
		if (FAILED(hr))
			throw runtime_error("Failed to retreive current thread system id.");
		return currentThreadSystemId;
	}


	auto frameToString(const PyFrame& frame) -> string
	{
		auto codeObject = frame.code();
		if (codeObject == nullptr)
			throw runtime_error("Warning: PyFrameObject is missing PyCodeObject.");

		auto filename = codeObject->filename();
		utils::ReprBuilder out(true);
		out.text("File \"").link(filename, ".open " + filename, "Open source code.")
			.text("\", line " + to_string(frame.currentLineNumber()) + ", in ")
			.link(codeObject->name(), "!pyobj 0n" + to_string(codeObject->offset()), "Inspect PyCodeObject.");
		return out.str();
	}


	auto frameToCommandString(const PyFrame& frame) -> string
	{
		utils::ReprBuilder out(true);

		auto* interpreterFrame = dynamic_cast<const PyInterpreterFrame*>(&frame);
		if (interpreterFrame) {
			// 3.11+
			out.link("[Frame]", "!pyinterpreterframe 0n"s + to_string(interpreterFrame->offset()), "Inspect interpreter frame (including localsplus).").text(" ");
		} else {
			// <3.11
			auto& frameObject = dynamic_cast<const PyFrameObject&>(frame);
			out.link("[Frame]", "!pyobj 0n"s + to_string(frameObject.offset()), "Inspect frame object (including localsplus).").text(" ");
		}

		auto locals = frame.locals();
		if (locals != nullptr && locals->offset() != 0)
			out.link("[Locals]", "!pyobj 0n"s + to_string(locals->offset()), "Inspect this frame's locals.").text(" ");

		auto globals = frame.globals();
		if (globals != nullptr && globals->offset() != 0)
			out.link("[Globals]", "!pyobj 0n"s + to_string(globals->offset()), "Inspect this frame's captured globals.").text(" ");

		return out.str();
	}

}

namespace PyExt {

	EXT_COMMAND(pystack,
		"Prints the Python stack for the current thread, or starting at a provided PyFrameObject."
		" Use -all to enumerate all Python threads.",
		"{all;b;;Print the Python stack for all Python threads.}"
		"{;s,o;PyFrameObject address}")
	{
		ensureSymbolsLoaded();

		// Prints the frame chain starting at `topFrame`, consuming the unique_ptr.
		auto printStack = [this](unique_ptr<PyFrame> frame) {
			for (; frame != nullptr; frame = frame->previous()) {
				// File and function names can hold non-ASCII text, which needs the wide output functions.
				Dml(L"\t%s\n", utils::utf8ToWide(frameToString(*frame)).c_str());
				Dml("\t\t%s\n", frameToCommandString(*frame).c_str());
			}
		};

		try {
			if (HasArg("all")) {
				for (auto&& istate : PyInterpreterState::allInterpreterStates()) {
					for (auto&& tstate : istate.allThreadStates()) {
						Out("Python Thread 0n%s:\n", to_string(tstate.thread_id()).c_str());
						try {
							auto frame = tstate.currentFrame();
							if (frame == nullptr)
								Out("\t<no Python frames>\n");
							else
								printStack(move(frame));
						} catch (exception& ex) {
							Warn("\t%s\n", ex.what());
						}
						Out("\n");
					}
				}
			}

			if (m_NumUnnamedArgs >= 1) {
				auto frameOffset = evalOffset(GetUnnamedArgStr(0));

				// The address is a frame object unless it turns out not to be a Python object at all,
				// in which case it's taken to be one of the bare interpreter frames Python 3.11+ uses.
				bool isFrameObject = false;
				utils::ignoreExtensionError([&] {
					isFrameObject = PyObject(frameOffset).type().name() == "frame";
				});

				unique_ptr<PyFrame> frame;
				if (isFrameObject) {
					Out("Stack trace starting at (PyFrameObject*)(%y):\n", frameOffset);
					frame = make_unique<PyFrameObject>(frameOffset);
				} else {
					Out("Stack trace starting at (_PyInterpreterFrame*)(%y):\n", frameOffset);
					frame = make_unique<PyInterpreterFrame>(RemoteType(frameOffset, "_PyInterpreterFrame"));
				}
				printStack(move(frame));
			} else if (!HasArg("all")) {
				// Print the thread header.
				auto threadHeader = "Thread " + to_string(getCurrentThreadId(m_System)) + ":";
				Out("%s\n", threadHeader.c_str());

				auto threadSystemId = getCurrentThreadSystemId(m_System);
				auto threadState = PyInterpreterState::findThreadStateBySystemThreadId(threadSystemId);
				if (!threadState.has_value())
					throw runtime_error("Thread does not contain any Python frames.");

				auto frame = threadState->currentFrame();
				if (frame == nullptr)
					throw runtime_error("Could not find PyFrameObject or PyInterpreterFrame.");
				printStack(move(frame));
			}
		} catch (exception& ex) {
			Warn("\t%s\n", ex.what());
			if (!HasFullMemBasic()) {
				Err("\tThe dump file does not contain enough data for PyExt to work properly.\n");
				Err("\tCapture a dump with full memory to ensure PyExt can reconstruct callstacks.\n");
			}
		}

		Out("\n");
	}

}
