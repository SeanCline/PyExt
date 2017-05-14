#pragma once

#include <engextcpp.hpp>

#include <vector>

// engextcpp uses this class as the basis for the entire extension.
// It's instantiated once in globals.cpp and a global pointer to it is held by engextcpp.
class EXT_CLASS : public ExtExtension
{

public:
	explicit EXT_CLASS();

public: // Commands.
	EXT_COMMAND_METHOD(pyobj);
	EXT_COMMAND_METHOD(pystack);

public: // Known structs.
	void KnownStructObjectHandler(_In_ PCSTR TypeName, _In_ ULONG Flags, _In_ ULONG64 Offset);

};