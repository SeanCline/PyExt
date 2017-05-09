#include "extension.h"

#include <engextcpp.hpp>

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
using namespace std;


vector<DEBUG_STACK_FRAME> EXT_CLASS::getStackFrames(int numFrames)
{
	std::vector<DEBUG_STACK_FRAME> frames(numFrames);
	ULONG framesFilled = 0;
	HRESULT hr = m_Control->GetStackTrace(0, 0, 0, frames.data(), frames.size(), &framesFilled);

	if (FAILED(hr))
		ThrowStatus(hr, "GetStackTrace failed.");

	frames.resize(framesFilled);

	return frames;
}

namespace {
	LONG getPyFrameSymbolIndex(IDebugSymbolGroup2* group)
	{
		ULONG numSymbols = 0;
		HRESULT hr = group->GetNumberSymbols(&numSymbols);
		if (FAILED(hr))
			return -1;

		for (ULONG i = 0; i < numSymbols; ++i) {
			std::vector<char> typeName(1024, '\0'); //< TODO: Make the size more dynamic.
			ULONG nameSize = 0;
			hr = group->GetSymbolTypeName(i, typeName.data(), typeName.size(), &nameSize);
			if (FAILED(hr))
				continue;

			if (typeName.data() == "struct _frame *"s)
				return i;
		}
		return -1;
	}
}


EXT_COMMAND(umstack, "Output the user-mode callstack stack", "")
{
	IDebugSymbolGroup2* group = nullptr;
	HRESULT hr = m_Symbols3->GetScopeSymbolGroup2(DEBUG_SCOPE_GROUP_LOCALS, nullptr, &group);
	if (FAILED(hr))
		Err("Initial GetScopeSymbolGroup2 failed.");

	auto frames = getStackFrames();
	for (auto& frame : frames) {
		// Print some stack information.
		Out("%lu %y\n", frame.FrameNumber, frame.InstructionOffset);


		// Print the function name.
		ExtBuffer<char> functionName;
		if (GetOffsetSymbol(frame.InstructionOffset, &functionName))
			Out("Name: %s\n", functionName.GetBuffer());

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
			// Print the symbol name.
			std::vector<char> name(1024, '\0'); //< TODO: Make the size more dynamic.
			ULONG nameSize = 0;
			hr = group->GetSymbolName(pyFrameSymbol, name.data(), name.size(), &nameSize);
			if (FAILED(hr)) {
				Warn("Failed to retreive symbol name.");
				continue;
			}
			name.resize(nameSize);

			// Print the symbol value.
			std::vector<char> value(1024, '\0'); //< TODO: Make the size more dynamic.
			ULONG valueSize = 0;
			hr = group->GetSymbolValueText(pyFrameSymbol, value.data(), value.size(), &valueSize);
			if (hr != S_OK) {
				Warn("Failed to retreive symbol value.");
				continue;
			}
			value.resize(valueSize);


			Out("Local: %s = %s\n", name.data(), value.data());
		}

		Out("\n");
	}

	if (group != nullptr) //< TODO: RAII
		group->Release();
}



