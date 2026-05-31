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


TEST_CASE("Pre-header layout on a GIL-build dump.", "[integration][build]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectTypesDumpFileNameOrDefault());

	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	// 64-bit dump: sizeof(_object) is 2*8 on GIL, 4*8 (give or take padding)
	// on free-threaded. The managed-dict slot sits at -3*ptr on GIL,
	// -1*ptr on free-threaded.
	REQUIRE(PyObject::headerSize() == 16);
	REQUIRE(PyObject::managedDictOffset() == -24);
}


TEST_CASE("Refcount and isImmortal on a GIL-build dump.", "[integration][build]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectTypesDumpFileNameOrDefault());

	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto pythonMinor = dump.pythonMinorVersion();

	auto frames = dump.getMainThreadFrames();
	auto bottomFrame = frames.back();
	auto locals = bottomFrame->locals();
	REQUIRE(locals != nullptr);
	auto pairs = locals->pairValues();

	SECTION("None has a sane positive refcount; immortal on 3.14+.")
	{
		auto& none = TestHelpers::findValueByKey(pairs, "none_obj");

		// Version-independent guard for the Phase-2 regression: ob_refcnt is a
		// uint32 on 3.14+ with the immortal sentinel's top bit set, so a naive
		// signed read returned a huge negative number. A positive refcount holds
		// on every Python version regardless of immortality.
		REQUIRE(none.refCount() > 0);

		// PEP 683 made None immortal in 3.12, but PyExt's isImmortal() only
		// decodes the 3.14 uint32 layout -- an 8-byte refcount read returns
		// false. So only assert immortality where the implementation detects it,
		// and pin the (known) negative result on the older detectable versions.
		// A 0 (unreadable) version falls through to the refcount guard above.
		if (pythonMinor >= 14)
			REQUIRE(none.isImmortal());
		else if (pythonMinor > 0)
			REQUIRE_FALSE(none.isImmortal());
	}

	SECTION("A user-allocated list has a small positive refcount and is not immortal.")
	{
		auto& list = TestHelpers::findValueByKey(pairs, "list_obj");

		REQUIRE(list.refCount() > 0);
		REQUIRE(list.refCount() < 1000);
		REQUIRE_FALSE(list.isImmortal());
	}
}
