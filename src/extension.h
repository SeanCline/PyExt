#pragma once

#include <engextcpp.hpp>
#include <exception>
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

		/// Prints UTF-8 encoded DML text followed by a newline, splitting it into chunks if needed.
		auto printDml(const std::string& content) -> void;

		/// Runs the body of a command, reporting a standard exception thrown by it the same way engextcpp
		/// reports its own. Left alone, such an exception would escape into the debugger as an unhandled C++ exception.
		template <typename Body>
		auto reportingStdExceptions(Body&& body) -> void
		{
			try {
				body();
			} catch (const std::exception& ex) {
				ThrowStatus(E_FAIL, "%s", ex.what());
			}
		}

		/// Evaluates an expression as a pointer and returns the result as an offset in the debuggee's address space.
		auto evalOffset(const std::string& arg) -> UINT64;

		/// Prints an error message to the user when Python symbols cannot be loaded.
		auto ensureSymbolsLoaded() -> void;
	};

}