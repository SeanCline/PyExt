#include "catch_amalgamated.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"

#include <globals.h>
#include <PyInterpreterState.h>
#include <PyThreadState.h>
#include <PyFrameObject.h>
#include <PyInterpreterFrame.h>
#include <PyCodeObject.h>
#include <PyTypeObject.h>
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>
#include <vector>
#include <algorithm>
#include <iterator>
#include <regex>
#include <ios>

TEST_CASE("fibonacci_test.py has the expected line numbers.", "[integration][fibonacci_test]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().fibonaciiDumpFileNameOrDefault());

	// Set up pyext.dll so it thinks DbgEng is calling into it.
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto frames = dump.getMainThreadFrames();
	REQUIRE(frames.size() > 90);

	SECTION("Bottom frame is the module.")
	{
		auto bottomFrame = frames.back();
		REQUIRE(bottomFrame->currentLineNumber() == 28);

		auto codeObj = bottomFrame->code();
		REQUIRE(codeObj != nullptr);
		REQUIRE(codeObj->name() == "<module>");
		REQUIRE(codeObj->filename().find("fibonacci_test.py") != std::string::npos);

		// Expected to be similar to: <code object, file ".\fibonacci_test.py", line 9>
		std::regex expectedRegex(R"(<code object, file "[^"]*fibonacci_test.py", line \d+>)");
		REQUIRE(regex_match(codeObj->repr(false), expectedRegex));
		// Expected to be similar to: "<link cmd="!pyobj 0n140729205561440">&lt;code object, file ".\fibonacci_test.py", line 9&gt;</link>"
		expectedRegex = R"(<link cmd="!pyobj 0n\d+">&lt;code object, file &quot;[^&]*fibonacci_test.py&quot;, line \d+&gt;</link>)";
		REQUIRE(regex_match(codeObj->repr(true), expectedRegex));
	}

	SECTION("The next several frames are in function recursive_fib.")
	{
		auto numFibFrames = std::count_if(begin(frames), end(frames), [](auto frame) {
			auto c = frame->code();
			return c != nullptr && c->name() == "recursive_fib" && frame->currentLineNumber() == 24;
		});

		REQUIRE(numFibFrames > 90);
	}

	SECTION("The top frame in recursive_fib is the one that triggered the dump.")
	{
		auto topFrameInFib = std::find_if(begin(frames), end(frames), [](auto frame) {
			auto c = frame->code();
			return c != nullptr && c->name() == "recursive_fib";
		});
		REQUIRE(topFrameInFib != frames.end());
		REQUIRE((*topFrameInFib)->currentLineNumber() == 18);
		auto dumpCode = (*(topFrameInFib - 1))->code();
		REQUIRE(dumpCode != nullptr);
		REQUIRE(dumpCode->name() == "dump_process");
	}

	SECTION("isIncomplete returns false for every captured frame.")
	{
		// All frames in the fibonacci dump are mid-execution and complete.
		// (inside recursive_fib or at the module-level call site)
		size_t interpFramesChecked = 0;
		for (auto const& frame : frames) {
			auto* interp = dynamic_cast<PyInterpreterFrame*>(frame.get()); //< Python <=3.11 has a different frame format.
			if (interp == nullptr)
				continue;
			REQUIRE_FALSE(interp->isIncomplete());
			++interpFramesChecked;
		}
		REQUIRE((interpFramesChecked == 0 || interpFramesChecked == frames.size()));
	}

	// Python 3.11 changed the co_linetable encoding, which broke our line-number lookup until it was reworked.
	// Now that the functionality is restored, this test prevents regressions in line number support.
	SECTION("currentLineNumber does not silently fall back to firstLineNumber.")
	{
		auto fibFrame = std::find_if(begin(frames), end(frames), [](auto frame) {
			auto c = frame->code();
			return c != nullptr && c->name() == "recursive_fib";
		});
		REQUIRE(fibFrame != frames.end());

		auto code = (*fibFrame)->code();
		REQUIRE(code != nullptr);

		auto currentLine = (*fibFrame)->currentLineNumber();
		auto firstLine = code->firstLineNumber();

		INFO("currentLineNumber=" << currentLine << ", firstLineNumber=" << firstLine);
		REQUIRE(currentLine != firstLine);
		REQUIRE(currentLine > firstLine);
	}

	// Similar regression guard for the bytecodeStartAddress().
	SECTION("bytecodeStartAddress returns a full 64-bit address.")
	{
		auto fibFrame = std::find_if(begin(frames), end(frames), [](auto frame) {
			auto c = frame->code();
			return c != nullptr && c->name() == "recursive_fib";
		});
		REQUIRE(fibFrame != frames.end());

		auto code = (*fibFrame)->code();
		REQUIRE(code != nullptr);

		auto start = code->bytecodeStartAddress();
		if (start.has_value()) {
			// The bytecode must live inside the PyCodeObject, so its address is strictly greater than the code object's own address.
			INFO("bytecodeStartAddress=0x" << std::hex << *start << ", code offset=0x" << std::hex << code->offset());
			REQUIRE(*start > code->offset());
		} else {
			// Python <= 3.10: no co_code_adaptive.
			// The old lineNumberFromInstructionOffset() path drives line numbers instead.
			SUCCEED("Skipping: bytecodeStartAddress not supported on this Python version.");
		}
	}
}