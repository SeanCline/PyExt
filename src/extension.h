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
		EXT_COMMAND_METHOD(pysymfix);
		EXT_COMMAND_METHOD(pysetautointerpreterstate);
		EXT_COMMAND_METHOD(pyinterpreterframe);

	public: // Known structs.
		auto KnownStructObjectHandler(_In_ PCSTR TypeName, _In_ ULONG Flags, _In_ ULONG64 Offset) -> void;

	private: // Helper methods.
		static const size_t chunkSize = 16000;

		/// Prints a DML text, splitting it into chunks if needed.
		auto printDml(const char* format, const std::string& content) -> void;

		/// Evaluates an expression as a pointer and returns the result as an offset in the debuggee's address space.
		auto evalOffset(const std::string& arg) -> UINT64;

		/// Prints an error message to the user when Python symbols cannot be loaded.
		auto ensureSymbolsLoaded() -> void;
	};

}