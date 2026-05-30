#include "catch_amalgamated.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"
#include "TestHelpers.h"

#include <globals.h>
#include <PyBuild.h>
#include <PyCodeObject.h>
#include <PyDictObject.h>
#include <PyFrame.h>
#include <PyInterpreterState.h>
#include <PyListObject.h>
#include <PyObject.h>
#include <PyThreadState.h>
#include <PyTypeObject.h>
using namespace PyExt::Remote;

#include <utils/ScopeExit.h>

#include <filesystem>
#include <stdexcept>
#include <string>


namespace {
	auto findLocal(
		const std::vector<std::pair<std::string, std::unique_ptr<PyExt::Remote::PyObject>>>& locals,
		const std::string& key) -> PyExt::Remote::PyObject&
	{
		for (auto& [name, obj] : locals) {
			if (name == key && obj != nullptr)
				return *obj;
		}
		throw std::runtime_error("Local not found for name=" + key);
	}
}


// Phase 7 — exercises the free-threaded code paths (detection, refcount
// split, pre-header offsets, dict-keys reads, stackref-decoded localsplus).
// SKIP cleanly when the dump file is missing so contributors without a
// free-threaded Python installed still get a green run.

namespace {
	auto loadFreeThreadedDumpOrSkip() -> PythonDumpFile
	{
		auto dumpPath = TestConfigData::instance().freeThreadedDumpFileNameOrDefault();
		if (!std::filesystem::exists(dumpPath))
			SKIP("Free-threaded dump not found at \"" + dumpPath + "\". "
			     "Install a python3.Xt and rerun free_threaded_test.py to generate it.");
		return PythonDumpFile(dumpPath);
	}
}


TEST_CASE("Free-threaded build is detected from the dump.", "[integration][free-threaded]")
{
	auto dump = loadFreeThreadedDumpOrSkip();

	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	// The Phase 1 probe (ob_tid on _Py_NoneStruct). Inverse of BuildTest's
	// GIL assertion — together they pin down the detection logic in both
	// directions.
	REQUIRE(PyExt::isFreeThreaded());

	// The Phase 3 pre-header helpers shift under FT. 64-bit dump: struct
	// _object roughly doubles (extra ob_tid/ob_mutex/split refcount), and
	// the managed-dict slot moves from -3*ptr to -1*ptr.
	REQUIRE(PyObject::headerSize() > 16);
	REQUIRE(PyObject::managedDictOffset() == -8);
}


TEST_CASE("Refcount split and immortality on a free-threaded dump.", "[integration][free-threaded]")
{
	auto dump = loadFreeThreadedDumpOrSkip();

	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto frames = dump.getMainThreadFrames();
	REQUIRE(frames.size() >= 2);

	// free_threaded_test.py captures from inside capture(); that function's
	// frame is at the bottom of the user-visible stack, just above the
	// win32debug.dump_process call site.
	std::shared_ptr<PyExt::Remote::PyFrame> captureFrame;
	for (auto& f : frames) {
		auto code = f->code();
		if (code != nullptr && code->name() == "capture") {
			captureFrame = f;
			break;
		}
	}
	REQUIRE(captureFrame != nullptr);

	auto locals = captureFrame->localsplus();

	SECTION("None local reads as immortal with a positive refcount.")
	{
		auto& none = findLocal(locals,"none_local");
		REQUIRE(none.refCount() > 0);
		REQUIRE(none.isImmortal());
	}

	SECTION("Fresh list local is mortal with a small refcount.")
	{
		auto& lst = findLocal(locals,"list_local");
		REQUIRE(lst.refCount() > 0);
		REQUIRE(lst.refCount() < 1000);
		REQUIRE_FALSE(lst.isImmortal());
	}
}


TEST_CASE("Stackref decoding resolves localsplus on a free-threaded dump.", "[integration][free-threaded]")
{
	auto dump = loadFreeThreadedDumpOrSkip();

	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	auto frames = dump.getMainThreadFrames();
	std::shared_ptr<PyExt::Remote::PyFrame> captureFrame;
	for (auto& f : frames) {
		auto code = f->code();
		if (code != nullptr && code->name() == "capture") {
			captureFrame = f;
			break;
		}
	}
	REQUIRE(captureFrame != nullptr);

	auto locals = captureFrame->localsplus();
	// Phase 6 gate: if stackref decoding falls back silently, the locals
	// either come back empty or are all-nullptr. Insist on at least one
	// resolved named entry.
	int resolved = 0;
	for (auto& [name, obj] : locals) {
		if (obj != nullptr && !name.empty())
			++resolved;
	}
	REQUIRE(resolved >= 1);

	SECTION("list_local round-trips through repr().")
	{
		auto& lst = dynamic_cast<PyListObject&>(findLocal(locals,"list_local"));
		REQUIRE(lst.type().name() == "list");
		REQUIRE(lst.numItems() == 3);
		REQUIRE(lst.repr(false) == "[ 1, 2, 3 ]");
	}

	SECTION("dict_local iterates via PyDictKeysObject under Py_GIL_DISABLED.")
	{
		auto& d = dynamic_cast<PyDictObject&>(findLocal(locals,"dict_local"));
		REQUIRE(d.type().name() == "dict");
		auto pairs = d.pairValues();
		REQUIRE(pairs.size() == 2);
	}
}
