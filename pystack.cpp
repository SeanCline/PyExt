#include "extension.h"

#include "objects/RemotePyFrameObject.h"
#include "objects/RemotePyDictObject.h"
#include "objects/RemotePyCodeObject.h"

#include "utils/ScopeExit.h"

#include <engextcpp.hpp>

#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>
using namespace std;


namespace {

	auto getPyFrameSymbolIndex(IDebugSymbolGroup2* pGroup) -> LONG
	{
		ULONG numSymbols = 0;
		HRESULT hr = pGroup->GetNumberSymbols(&numSymbols);
		if (FAILED(hr))
			return -1;

		for (ULONG i = 0; i < numSymbols; ++i) {
			const auto bufferSize = 1024u; //< I don't imagine the _frame's typename will ever be this long.
			vector<char> typeName(bufferSize, '\0');
			ULONG nameSize = 0;
			hr = pGroup->GetSymbolTypeName(i, typeName.data(), bufferSize, &nameSize);
			if (FAILED(hr))
				continue;

			if ("struct _frame *"s == typeName.data())
				return i;
		}
		return -1;
	}


	auto getNativeStackFrames(IDebugControl* pControl, size_t numFrames = 1024) -> vector<DEBUG_STACK_FRAME>
	{
		vector<DEBUG_STACK_FRAME> frames(numFrames);
		ULONG framesFilled = 0;
		HRESULT hr = pControl->GetStackTrace(0, 0, 0, frames.data(), static_cast<ULONG>(frames.size()), &framesFilled);

		if (FAILED(hr))
			runtime_error("GetStackTrace failed with hr="s + to_string(hr));

		frames.resize(framesFilled);

		return frames;
	}


	ULONG64 DereferenceRemotePointer(ULONG64 ptr) {
		return ExtRemoteData(ptr, g_Ext->m_PtrSize).GetPtr();
	}


	vector<RemotePyFrameObject> getPythonFrames(IDebugSymbols3* pSymbols, const vector<DEBUG_STACK_FRAME>& nativeFrames)
	{
		vector<RemotePyFrameObject> frameObjects;
		IDebugSymbolGroup2* group = nullptr;
		HRESULT hr = pSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, nullptr, &group);

		auto releaseGroup = utils::makeScopeExit([&group] {
			if (group != nullptr) {
				group->Release();
				group = nullptr;
			}
		});

		if (FAILED(hr))
			throw runtime_error("Initial GetScopeSymbolGroup2 failed.");

		for (auto& nativeFrame : nativeFrames) {
			// Make sure this is an EvalFrameEx call.
			int bufferSize = 1024;
			vector<char> functionNameBuffer(1024, '\0');
			hr = pSymbols->GetNameByOffset(nativeFrame.InstructionOffset, functionNameBuffer.data(), bufferSize, nullptr, nullptr);
			if (FAILED(hr) || string(functionNameBuffer.data()).find("EvalFrameEx") == string::npos) {
				continue;
			}

			// Set the symbol scope to the current frame.
			hr = pSymbols->SetScope(nativeFrame.InstructionOffset, const_cast<DEBUG_STACK_FRAME*>(&nativeFrame), nullptr, 0);
			if (FAILED(hr))
				throw runtime_error("SetScope failed with hr=");

			// Only look at locals.
			hr = pSymbols->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, group, &group);
			if (FAILED(hr))
				throw runtime_error("GetScopeSymbolGroup2 failed with hr=");

			// Look for a python _frame pointer.
			auto pyFrameSymbol = getPyFrameSymbolIndex(group);
			if (pyFrameSymbol >= 0) {
				RemotePyFrameObject::Offset offset = 0;
				hr = group->GetSymbolOffset(pyFrameSymbol, &offset);
				if (FAILED(hr)) {
					::OutputDebugStringA("Failed to retreive frame offset.");
					continue;
				}

				// Put it on the list.
				frameObjects.emplace_back(DereferenceRemotePointer(offset));
			}
		}

		return frameObjects;
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
	auto nativeFrames = getNativeStackFrames(m_Control);
	auto frameObjects = getPythonFrames(m_Symbols3, nativeFrames);

	// Print the thread header.
	ULONG threadId = 0xFFFF;
	m_System->GetCurrentThreadId(&threadId);
	auto threadHeader = "Thread " + to_string(threadId) + ":";
	Out("%s\n", threadHeader.c_str());

	// Make sure there are frames.
	if (frameObjects.empty()) {
		Warn("\tThread does not contain any Python frames.\n\n", threadHeader.c_str());
		return;
	}

	// Print each frame.
	for (auto& frameObject : frameObjects) {
		try {
			auto frameStr = frameToString(frameObject);
			Out("\t%s\n", frameStr.c_str());

			//auto frameCommandStr = frameToCommandString(frameObject);
			//Dml("\t%s\n", frameCommandStr.c_str());
		} catch (exception& ex) {
			Warn("%s\n", ex.what());
		}
	}

	Out("\n");
}
