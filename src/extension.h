#pragma once

#include <engextcpp.hpp>
#include <string>


namespace PyExt {

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
		auto KnownStructObjectHandler(_In_ PCSTR TypeName, _In_ ULONG Flags, _In_ ULONG64 Offset) -> void;

	private: // Helper methods.
		/// Evaluates an expression as a pointer and returns the result as an offset in the debuggee's address space.
		auto evalOffset(const std::string& arg) -> UINT64;

		/// Prints an error message to the user  when Python symbols cannot be loaded.
		auto ensureSymbolsLoaded() -> void;
	};

}