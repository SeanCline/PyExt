#include "catch_amalgamated.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"

#include <globals.h>
#include <PyBuild.h>

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
