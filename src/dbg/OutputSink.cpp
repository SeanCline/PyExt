// Implementation of the printf-passthrough OutputSink (engextcpp-replacement.md
// §5). Does NOT use the precompiled header (no engextcpp). The ControlOutputSink
// channel mapping is copied verbatim from engextcpp's ExtExtension::Out/Warn/
// Err/Dml so output is byte-for-byte identical.

#include "OutputSink.h"

#include <windows.h>
#include <DbgEng.h>

namespace PyExt::Dbg {

	// --- variadic forwarders: identical call shape to ExtExtension::Out/etc. ---

	void OutputSink::out(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt); vout(fmt, args); va_end(args);
	}

	void OutputSink::dml(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt); vdml(fmt, args); va_end(args);
	}

	void OutputSink::warn(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt); vwarn(fmt, args); va_end(args);
	}

	void OutputSink::err(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt); verr(fmt, args); va_end(args);
	}


	// --- ControlOutputSink: same DbgEng engine + masks engextcpp uses ---

	void ControlOutputSink::vout(const char* fmt, va_list args)
	{
		control_->OutputVaList(DEBUG_OUTPUT_NORMAL, fmt, args);
	}

	void ControlOutputSink::vdml(const char* fmt, va_list args)
	{
		control_->ControlledOutputVaList(DEBUG_OUTCTL_AMBIENT_DML, DEBUG_OUTPUT_NORMAL, fmt, args);
	}

	void ControlOutputSink::vwarn(const char* fmt, va_list args)
	{
		control_->OutputVaList(DEBUG_OUTPUT_WARNING, fmt, args);
	}

	void ControlOutputSink::verr(const char* fmt, va_list args)
	{
		control_->OutputVaList(DEBUG_OUTPUT_ERROR, fmt, args);
	}

}
