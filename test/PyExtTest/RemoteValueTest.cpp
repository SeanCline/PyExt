// Parity test for the raw-DbgEng RemoteValue (engextcpp-replacement.md Phase 2 /
// SPIKE.md). Ground truth comes from the existing exported PyExt object model
// (which reads through ExtRemoteTyped under the hood). Every RemoteValue read
// must match it on a live dump.

#include "catch_amalgamated.hpp"

#include "PythonDumpFile.h"
#include "TestConfigData.h"

#include <globals.h>
#include <PyObject.h>
#include <PyTypeObject.h>
#include <utils/ScopeExit.h>

#include "../../src/dbg/DbgEngContext.h"
#include "../../src/dbg/RemoteValue.h"

#include <cstdint>
using namespace PyExt::Dbg;
using namespace PyExt::Remote;


TEST_CASE("RemoteValue matches the ExtRemoteTyped object model.", "[integration][remotevalue]")
{
	auto dump = PythonDumpFile(TestConfigData::instance().objectDetailsDumpFileNameOrDefault());
	PyExt::InitializeGlobalsForTest(dump.pClient.Get());
	auto cleanup = utils::makeScopeExit(PyExt::UninitializeGlobalsForTest);

	DbgEngContext ctx(dump.pClient.Get());

	SECTION("pointerSize / evaluate / qualify")
	{
		REQUIRE(ctx.pointerSize() == 8);

		auto addr = ctx.evaluateU64("python314!PyType_Type");
		REQUIRE(addr.has_value());
		REQUIRE(*addr != 0);

		auto qualified = ctx.qualifyPythonSymbol("ob_type");
		REQUIRE(qualified.has_value());
		REQUIRE(qualified->find("!ob_type") != std::string::npos);
	}

	SECTION("scalar + field parity against the object model")
	{
		auto addrOpt = ctx.evaluateU64("python314!PyType_Type");
		REQUIRE(addrOpt.has_value());
		const std::uint64_t addr = *addrOpt;

		// Ground truth via the exported (ExtRemoteTyped-backed) object model.
		auto obj = PyObject::make(addr);
		const auto truthRefcnt = obj->refCount();
		const auto truthTypePtr = obj->type().offset();

		// New raw-DbgEng layer.
		RemoteValue rv(ctx, "_object", addr);

		auto refcntField = rv.baseField("ob_refcnt");
		REQUIRE(refcntField.has_value());
		REQUIRE(refcntField->as<std::int64_t>() == truthRefcnt);

		auto typeField = rv.baseField("ob_type");
		REQUIRE(typeField.has_value());
		REQUIRE(typeField->ptr() == truthTypePtr);
	}

	SECTION("typeSize via GetTypeSize (the cast-expression-site primitive)")
	{
		auto addr = ctx.evaluateU64("python314!PyType_Type");
		REQUIRE(addr.has_value());
		// PyObject header is 2 pointers on a GIL build (proven 0x10 in Phase 0).
		REQUIRE(RemoteValue(ctx, "PyObject", *addr).typeSize()
			== static_cast<std::size_t>(2 * ctx.pointerSize()));
	}

	SECTION("expected-absent field returns nullopt, does not throw")
	{
		auto addr = ctx.evaluateU64("python314!PyType_Type");
		REQUIRE(addr.has_value());
		RemoteValue rv(ctx, "_object", *addr);
		REQUIRE_FALSE(rv.field("field_that_does_not_exist").has_value());
	}
}
