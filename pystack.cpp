#include "extension.h"

#include "objects/RemotePyFrameObject.h"
#include "objects/RemotePyDictObject.h"
#include "objects/RemotePyCodeObject.h"
#include "utils/ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
#include <utility>
#include <sstream>
#include <stdexcept>
using namespace std;


namespace {
	unique_ptr<RemotePyFrameObject> getPythonFrameForThread(uint64_t threadId)
	{
		auto threadState = ExtRemoteTyped("autoInterpreterState").Field("tstate_head");

		// Iterate over each threadState, looking for one with a matching threadId.
		for (; threadState.GetPtr() != 0; threadState = threadState.Field("next")) {
			auto threadIdField = threadState.Field("thread_id");
			auto curThreadId = utils::readIntegral<uint64_t>(threadIdField);
			if (curThreadId == threadId)
				return make_unique<RemotePyFrameObject>(threadState.Field("frame").GetPtr()); //< Found it!
		}

		return { };
	}


	uint64_t getCurrentThreadId(IDebugSystemObjects* pSystem)
	{
		ULONG currentThreadSystemId = 0;
		HRESULT hr = pSystem->GetCurrentThreadId(&currentThreadSystemId);
		if (FAILED(hr))
			throw runtime_error("Failed to retreive current thread id.");
		return currentThreadSystemId;
	}


	uint64_t getCurrentThreadSystemId(IDebugSystemObjects* pSystem)
	{
		ULONG currentThreadSystemId = 0;
		HRESULT hr = pSystem->GetCurrentThreadSystemId(&currentThreadSystemId);
		if (FAILED(hr))
			throw runtime_error("Failed to retreive current thread system id.");
		return currentThreadSystemId;
	}


	string frameToString(const RemotePyFrameObject& frameObject)
	{
		ostringstream oss;

		auto codeObject = frameObject.code();
		if (codeObject != nullptr) {
			oss << "File \"" << codeObject->filename() << "\"";
			oss << ", line " << frameObject.currentLineNumber();
			oss << ", in " << codeObject->name();
		} else {
			// This shouldn't ever happen.
			throw runtime_error("Warning: PyFrameObject is missing PyCodeObject.");
		}

		return oss.str();
	}

	/*
	string frameToCommandString(const RemotePyFrameObject& frameObject)
	{
		ostringstream oss;
		auto locals = frameObject.locals();
		if (locals != nullptr && locals->offset() != 0)
			oss << "<link cmd=\"!pyobj 0x" << hex << locals->offset() << "\">[Locals]</link>";

		auto code = frameObject.code();
		if (code != nullptr && code->offset() != 0)
			oss << "<link cmd=\"!pyobj 0x" << hex << code->offset() << "\">[code]</link>";

		return oss.str();
	}
	*/
}


EXT_COMMAND(pystack, "Output the Python stack for the current thread.", "")
{
	try {
		// Print the thread header.
		auto threadId = getCurrentThreadId(m_System);
		auto threadHeader = "Thread " + to_string(threadId) + ":";
		Out("%s\n", threadHeader.c_str());

		auto threadSystemId = getCurrentThreadSystemId(m_System);
		auto frameObject = getPythonFrameForThread(threadSystemId);
		if (frameObject == nullptr)
			throw runtime_error("Thread does not contain any Python frames.");

		// Print each frame.
		for (; frameObject != nullptr; frameObject = frameObject->back()) {
			auto frameStr = frameToString(*frameObject);
			Out("\t%s\n", frameStr.c_str());

			//auto frameCommandStr = frameToCommandString(frameObject);
			//Dml("\t%s\n", frameCommandStr.c_str());
		}

	} catch (exception& ex) {
		Warn("\t%s\n\n", ex.what());
	}

	Out("\n");
}
