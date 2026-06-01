#pragma once

// Replaces engextcpp's g_Ext singleton (engextcpp-replacement.md §7.1/§7.6).
//
// A DbgEngContext bundles the COM interfaces a command needs and is built from
// the IDebugClient* that DbgEng hands to each command export. It is passed
// explicitly (not a global), which is what makes the data layer unit-testable:
// the test harness already owns an IDebugClient* (see InitializeGlobalsForTest)
// and can construct a context + a CaptureSink with no singleton involved.

#include "OutputSink.h"
#include "pyextpublic.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

struct IDebugClient;
struct IDebugControl;
struct IDebugSymbols3;
struct IDebugDataSpaces4;
struct IDebugSystemObjects;

namespace PyExt::Dbg {

	class PYEXT_PUBLIC DbgEngContext {
	public:
		// QIs the client for the interfaces we use; throws if any QI fails.
		// `sink` defaults to a ControlOutputSink over the queried IDebugControl.
		explicit DbgEngContext(IDebugClient* client, std::shared_ptr<OutputSink> sink = nullptr);
		~DbgEngContext();

		DbgEngContext(const DbgEngContext&) = delete;
		DbgEngContext& operator=(const DbgEngContext&) = delete;

		// Raw interface access for the corners that still need it
		// (symbol reload, module enumeration, expression eval).
		auto control() const -> IDebugControl*;
		auto symbols() const -> IDebugSymbols3*;
		auto data() const -> IDebugDataSpaces4*;
		auto system() const -> IDebugSystemObjects*;

		auto output() const -> OutputSink&;

		// The one place the DbgEng C++ expression evaluator is still used
		// (engextcpp-replacement.md §2): user-supplied expressions and the
		// handful of `sizeof`/cast exprs that aren't worth hand-lowering.
		// Wraps IDebugControl::Evaluate(..., DEBUG_VALUE_INT64). nullopt on a
		// syntactically/semantically invalid expression (replaces the
		// try/catch around ExtRemoteTyped in evalOffset).
		auto evaluateU64(std::string_view expression) const -> std::optional<std::uint64_t>;

		// 4 on x86 debuggees, 8 on x64. Replaces utils::getPointerSize()'s
		// "sizeof(void*)" trick with IDebugControl::IsPointer64Bit().
		auto pointerSize() const -> int;

		// "python314!ob_type" style qualified name. Replaces
		// utils::getFullSymbolName + FindFirstModule("python???").
		auto qualifyPythonSymbol(std::string_view symbol) const -> std::optional<std::string>;

	private:
		struct Impl;
		std::unique_ptr<Impl> impl_;
	};

}
