#include "PyBuild.h"

#include <engextcpp.hpp>

namespace PyExt {

	auto isFreeThreaded() -> bool
	{
		// PEP 703 added `ob_tid` (owning thread id) to `struct _object`.
		// The field is absent on a GIL build, so its presence is the cleanest
		// build-mode discriminator we have without picking a Python version.
		//
		// Probe via the None singleton: `_Py_NoneStruct` is always live and
		// is statically typed as `PyObject`, so the symbol resolves without
		// needing a heap address from the dump.
		try {
			ExtRemoteTyped none("_Py_NoneStruct");
			return none.HasField("ob_tid");
		} catch (...) {
			return false;
		}
	}


	auto warnIfFreeThreaded() -> void
	{
		if (!isFreeThreaded())
			return;
		g_Ext->Warn(
			"Note: free-threaded (Py_GIL_DISABLED) build detected. PyExt's "
			"support for free-threaded interpreters is in progress; the "
			"following may be inaccurate or missing in this output: refcounts, "
			"managed dicts, frame localsplus, generator/coroutine state.\n");
	}

}
