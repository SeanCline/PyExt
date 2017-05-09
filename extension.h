#pragma once

#include <engextcpp.hpp>

#include <vector>

//----------------------------------------------------------------------------
//
// Base extension class.
// Extensions derive from the provided ExtExtension class.
//
// The standard class name is "Extension".  It can be
// overridden by providing an alternate definition of
// EXT_CLASS before including engextcpp.hpp.
//
//----------------------------------------------------------------------------
class EXT_CLASS : public ExtExtension
{
public:
	explicit EXT_CLASS();

public: // Commands.
	EXT_COMMAND_METHOD(pyobj);
	EXT_COMMAND_METHOD(ummods);
	EXT_COMMAND_METHOD(umstack);

public: // Known structs.
	void KnownStructObjectHandler(_In_ PCSTR TypeName, _In_ ULONG Flags, _In_ ULONG64 Offset);

private:
	std::vector<DEBUG_STACK_FRAME> getStackFrames(int numFrames = 1024);
};