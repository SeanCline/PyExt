#pragma once

// Output redirection for the engextcpp-free extension (engextcpp-replacement.md
// §5 / §7.5). Everything that prints today via ExtExtension::Out/Dml/Warn/Err
// or that suppresses output via ExtCaptureOutputA routes through an OutputSink.
//
// Design notes:
//  - The sink takes already-formatted text. Callers format with std::format
//    (the project builds with /std:c++latest), replacing the printf-style
//    Out("%s", x.c_str()) idiom with out(std::format("{}", x)).
//  - Channels mirror DbgEng's output masks: normal / DML / warning / error.
//  - Redirection is just swapping the sink — no DbgEng capture object needed.

#include <string>
#include <string_view>

struct IDebugControl;

namespace PyExt::Dbg {

	class OutputSink {
	public:
		virtual ~OutputSink() = default;

		virtual void out(std::string_view text) = 0;   // DEBUG_OUTPUT_NORMAL
		virtual void dml(std::string_view text) = 0;    // DML-formatted normal output
		virtual void warn(std::string_view text) = 0;   // DEBUG_OUTPUT_WARNING
		virtual void err(std::string_view text) = 0;     // DEBUG_OUTPUT_ERROR
	};


	// Default sink: forwards to the live debugger via IDebugControl::Output /
	// ControlledOutput. Replaces the implicit g_Ext->Out/Dml/Warn/Err path.
	// (impl in OutputSink.cpp — see Phase 1)
	class ControlOutputSink final : public OutputSink {
	public:
		explicit ControlOutputSink(IDebugControl* control);
		void out(std::string_view text) override;
		void dml(std::string_view text) override;
		void warn(std::string_view text) override;
		void err(std::string_view text) override;
	private:
		IDebugControl* control_;
	};


	// Discards everything. Replaces utils::ignoreExtensionError's use of
	// ExtCaptureOutputA when we only want to suppress noise (e.g.
	// ensureSymbolsLoaded's probing).
	class NullSink final : public OutputSink {
	public:
		void out(std::string_view) override {}
		void dml(std::string_view) override {}
		void warn(std::string_view) override {}
		void err(std::string_view) override {}
	};


	// Collects output for assertions. Replaces the need to scrape WinDbg output
	// in the test harness.
	class CaptureSink final : public OutputSink {
	public:
		void out(std::string_view text) override { normal_ += text; }
		void dml(std::string_view text) override { normal_ += text; }
		void warn(std::string_view text) override { warnings_ += text; }
		void err(std::string_view text) override { errors_ += text; }

		auto text() const -> const std::string& { return normal_; }
		auto warnings() const -> const std::string& { return warnings_; }
		auto errors() const -> const std::string& { return errors_; }
		void clear() { normal_.clear(); warnings_.clear(); errors_.clear(); }
	private:
		std::string normal_, warnings_, errors_;
	};

}
