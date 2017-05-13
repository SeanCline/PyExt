#include "extension.h"

#include "objects/RemotePyFrameObject.h"
#include "objects/RemotePyDictObject.h"
#include "objects/RemotePyCodeObject.h"

#include <engextcpp.hpp>

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
using namespace std;


auto EXT_CLASS::getStackFrames(size_t numFrames) -> vector<DEBUG_STACK_FRAME>
{
	vector<DEBUG_STACK_FRAME> frames(numFrames);
	ULONG framesFilled = 0;
	HRESULT hr = m_Control->GetStackTrace(0, 0, 0, frames.data(), static_cast<ULONG>(frames.size()), &framesFilled);

	if (FAILED(hr))
		ThrowStatus(hr, "GetStackTrace failed.");

	frames.resize(framesFilled);

	return frames;
}

namespace {
	auto getPyFrameSymbolIndex(IDebugSymbolGroup2* group) -> LONG
	{
		ULONG numSymbols = 0;
		HRESULT hr = group->GetNumberSymbols(&numSymbols);
		if (FAILED(hr))
			return -1;

		for (ULONG i = 0; i < numSymbols; ++i) {
			const auto bufferSize = 1024u; //< I don't imagine the _frame's typename will ever be this long.
			vector<char> typeName(bufferSize, '\0');
			ULONG nameSize = 0;
			hr = group->GetSymbolTypeName(i, typeName.data(), bufferSize, &nameSize);
			if (FAILED(hr))
				continue;

			if ("struct _frame *"s == typeName.data())
				return i;
		}
		return -1;
	}

	ULONG64 DereferenceRemotePointer(ULONG64 ptr) {
		return ExtRemoteData(ptr, g_Ext->m_PtrSize).GetPtr();
	}
}


EXT_COMMAND(pystack, "Output the Python stack for the current thread.", "")
{
	IDebugSymbolGroup2* group = nullptr;
	HRESULT hr = m_Symbols3->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, nullptr, &group);
	if (FAILED(hr))
		Err("Initial GetScopeSymbolGroup2 failed.");

	auto frames = getStackFrames();
	for (auto& frame : frames) {
		// Set the symbol scope to the current frame.
		hr = m_Symbols->SetScope(frame.InstructionOffset, &frame, nullptr, 0);
		if (FAILED(hr))
			ThrowStatus(hr, "SetScope failed.");

		// Only look at locals.
		hr = m_Symbols3->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, group, &group);
		if (FAILED(hr))
			ThrowStatus(hr, "GetScopeSymbolGroup2 failed.");

		// Look for a python _frame pointer.
		auto pyFrameSymbol = getPyFrameSymbolIndex(group);
		if (pyFrameSymbol >= 0) {
			RemotePyFrameObject::Offset offset = 0;
			hr = group->GetSymbolOffset(pyFrameSymbol, &offset);
			if (FAILED(hr)) {
				Warn("Failed to retreive frame offset.");
				continue;
			}

			auto frameObject = RemotePyFrameObject(DereferenceRemotePointer(offset));
			//Out("Frame object: %s\n", frameObject.repr().c_str());

			auto codeObject = frameObject.code();
			if (codeObject != nullptr) {
				Out("Code object: %s\n", codeObject->repr().c_str());
				Out("Function name: %s\n", codeObject->name().c_str());
				Out("File %s @ %d\n", codeObject->filename().c_str(), frameObject.currentLineNumber());
			}

			auto localsObject = frameObject.locals();
			if (localsObject != nullptr)
				Out("Locals: %s\n", localsObject->repr().c_str());

			Out("\n");
		}
	}

	if (group != nullptr) //< TODO: RAII
		group->Release();
}
