#include "catch_amalgamated.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"

#include <globals.h>
#include <PyInterpreterState.h>
#include <PyThreadState.h>
#include <PyFrame.h>
#include <PyCodeObject.h>
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>
using namespace std;


namespace {
	/// Returns true if any frame in the thread's stack has the given function name.
	auto threadHasFunction(const PyThreadState& tstate, const string& funcName) -> bool
	{
		for (auto frame = tstate.currentFrame(); frame != nullptr; frame = frame->previous()) {
			auto code = frame->code();
			if (code != nullptr && code->name() == funcName)
				return true;
		}
		return false;
	}
}


TEST_CASE("pystack_all_test.py has multiple Python threads with expected stacks.", "[integration][pystack_all]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().pystackAllDumpFileNameOrDefault());

	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	// Collect all thread states across all interpreter states.
	vector<PyThreadState> allThreads;
	for (auto&& istate : PyInterpreterState::allInterpreterStates()) {
		for (auto&& tstate : istate.allThreadStates()) {
			allThreads.push_back(move(tstate));
		}
	}

	SECTION("More than one Python thread is present.")
	{
		REQUIRE(allThreads.size() > 1);
	}

	SECTION("At least one thread has worker_function on its stack.")
	{
		auto workerCount = count_if(begin(allThreads), end(allThreads), [](const PyThreadState& ts) {
			return threadHasFunction(ts, "worker_function");
		});
		REQUIRE(workerCount >= 1);
	}

	SECTION("At least one thread has the module-level frame on its stack.")
	{
		auto moduleCount = count_if(begin(allThreads), end(allThreads), [](const PyThreadState& ts) {
			return threadHasFunction(ts, "<module>");
		});
		REQUIRE(moduleCount >= 1);
	}
}
