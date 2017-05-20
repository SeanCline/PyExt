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
	auto getPythonFrameForThread(uint64_t threadId) -> unique_ptr<RemotePyFrameObject>
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


	auto escapeDml(const string& str) -> string
	{
		std::string buffer;
		buffer.reserve(str.size());
		for (auto ch : str) {
			switch (ch) {
				case '&':  buffer += "&amp;";  break;
				case '\"': buffer += "&quot;"; break;
				case '\'': buffer += "&apos;"; break;
				case '<':  buffer += "&lt;";   break;
				case '>':  buffer += "&gt;";   break;
				default:   buffer += ch;       break;
			}
		}
		return buffer;
	}

	auto link(const string& text, const string& cmd, const string& alt = ""s) -> string
	{
		ostringstream oss;
		oss << "<link cmd=\"" << escapeDml(cmd) << "\"";
		if (!alt.empty())
			oss << " alt=\"" << escapeDml(alt) << "\"";
		oss << ">" << escapeDml(text) << "</link>";
		return oss.str();
	}

	auto frameToString(const RemotePyFrameObject& frameObject) -> string
	{
		ostringstream oss;

		auto codeObject = frameObject.code();
		if (codeObject == nullptr)
			throw runtime_error("Warning: PyFrameObject is missing PyCodeObject.");

		auto filename = codeObject->filename();
		oss << "File \"" << link(filename, ".open " + filename, "Open source code.")
			<< "\", line " << frameObject.currentLineNumber()
			<< ", in " << link(codeObject->name(), "!pyobj 0n" + to_string(codeObject->offset()), "Inspect PyCodeObject.");

		return oss.str();
	}


	auto frameToCommandString(const RemotePyFrameObject& frameObject) -> string
	{
		ostringstream oss;

		auto locals = frameObject.locals();
		if (locals != nullptr && locals->offset() != 0)
			oss << link("[Locals]", "!pyobj 0n"s + to_string(locals->offset()), "Inspect this frame's locals.") << " ";

		auto globals = frameObject.globals();
		if (globals != nullptr && globals->offset() != 0)
			oss << link("[Globals]", "!pyobj 0n"s + to_string(globals->offset()), "Inspect this frame's captured globals.") << " ";

		return oss.str();
	}

}


EXT_COMMAND(pystack, "Output the Python stack for the current thread.", "")
{
	ensureSymbolsLoaded();

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
			Dml("\t%s\n", frameStr.c_str());

			auto frameCommandStr = frameToCommandString(*frameObject);
			Dml("\t\t%s\n", frameCommandStr.c_str());
		}

	} catch (exception& ex) {
		Warn("\t%s\n\n", ex.what());
	}

	Out("\n");
}
