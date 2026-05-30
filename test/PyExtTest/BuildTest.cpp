#include "catch_amalgamated.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"
#include "TestHelpers.h"

#include <globals.h>
#include <PyBuild.h>
#include <PyDictObject.h>
#include <PyFrame.h>
#include <PyObject.h>
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>


TEST_CASE("isFreeThreaded() returns false against a GIL-build dump.", "[integration][build]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectTypesDumpFileNameOrDefault());

	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	// All shipped test dumps come from the python.org GIL build, so the probe
	// must say so. A free-threaded dump (when one exists) would assert true.
	REQUIRE_FALSE(PyExt::isFreeThreaded());
}


TEST_CASE("Refcount and isImmortal on a GIL-build dump.", "[integration][build]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectTypesDumpFileNameOrDefault());

	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto frames = dump.getMainThreadFrames();
	auto bottomFrame = frames.back();
	auto locals = bottomFrame->locals();
	REQUIRE(locals != nullptr);
	auto pairs = locals->pairValues();

	SECTION("None is immortal with a large positive refcount.")
	{
		auto& none = TestHelpers::findValueByKey(pairs, "none_obj");

		// Catches the pre-Phase-2 regression: ob_refcnt is uint32 on 3.14+ and
		// the immortal sentinel has the top bit set, so a naive signed read
		// returned a huge negative number.
		REQUIRE(none.refCount() > 0);
		REQUIRE(none.isImmortal());
	}

	SECTION("A user-allocated list has a small positive refcount and is not immortal.")
	{
		auto& list = TestHelpers::findValueByKey(pairs, "list_obj");

		REQUIRE(list.refCount() > 0);
		REQUIRE(list.refCount() < 1000);
		REQUIRE_FALSE(list.isImmortal());
	}
}
