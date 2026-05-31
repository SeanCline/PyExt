#pragma once

// Output redirection for the engextcpp-free extension (engextcpp-replacement.md
// §5 / §7.5). Everything that prints today via ExtExtension::Out/Dml/Warn/Err
// routes through an OutputSink instead.
//
// IMPORTANT — byte-for-byte parity (Phase 1 gate):
// The sink is a *printf passthrough* over the exact same DbgEng formatter
// engextcpp uses, NOT a std::format wrapper. DbgEng format strings carry
// engine-specific specifiers (%y = symbol+offset, %p, DML <link> markup) that
// std::format cannot reproduce. Each method forwards an unchanged format string
// + va_list to IDebugControl::OutputVaList / ControlledOutputVaList, mirroring
// engextcpp's ExtExtension::Out/Warn/Err/Dml verbatim:
//   Out  -> OutputVaList(DEBUG_OUTPUT_NORMAL, fmt, args)
//   Warn -> OutputVaList(DEBUG_OUTPUT_WARNING, fmt, args)
//   Err  -> OutputVaList(DEBUG_OUTPUT_ERROR, fmt, args)
//   Dml  -> ControlledOutputVaList(DEBUG_OUTCTL_AMBIENT_DML, DEBUG_OUTPUT_NORMAL, fmt, args)
// (m_OutMask is DEBUG_OUTPUT_NORMAL for command output, so NORMAL is exact.)

#include <cstdarg>
#include <sal.h> // SAL annotations (_In_z_, _Printf_format_string_)

struct IDebugControl;

namespace PyExt::Dbg {

	class OutputSink {
	public:
		virtual ~OutputSink() = default;

		// printf-style, identical call shape to ExtExtension::Out/Dml/Warn/Err.
		void out(_In_z_ _Printf_format_string_ const char* fmt, ...);
		void dml(_In_z_ _Printf_format_string_ const char* fmt, ...);
		void warn(_In_z_ _Printf_format_string_ const char* fmt, ...);
		void err(_In_z_ _Printf_format_string_ const char* fmt, ...);

	protected:
		// Channels a concrete sink implements. Variadic forwarders above call these.
		virtual void vout(const char* fmt, va_list args) = 0;
		virtual void vdml(const char* fmt, va_list args) = 0;
		virtual void vwarn(const char* fmt, va_list args) = 0;
		virtual void verr(const char* fmt, va_list args) = 0;
	};


	// Default sink: forwards to the live debugger via the same VaList calls
	// engextcpp makes. Construct from the command's IDebugControl (m_Control).
	class ControlOutputSink final : public OutputSink {
	public:
		explicit ControlOutputSink(IDebugControl* control) : control_(control) {}
	protected:
		void vout(const char* fmt, va_list args) override;
		void vdml(const char* fmt, va_list args) override;
		void vwarn(const char* fmt, va_list args) override;
		void verr(const char* fmt, va_list args) override;
	private:
		IDebugControl* control_;
	};


	// Discards everything. For the "suppress this output" paths (eventually
	// replacing ExtCaptureOutputA in ensureSymbolsLoaded / ignoreExtensionError).
	class NullSink final : public OutputSink {
	protected:
		void vout(const char*, va_list) override {}
		void vdml(const char*, va_list) override {}
		void vwarn(const char*, va_list) override {}
		void verr(const char*, va_list) override {}
	};

}
