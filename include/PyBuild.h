#pragma once

#include "pyextpublic.h"

namespace PyExt {

	/// Returns true when the loaded Python interpreter is a free-threaded
	/// (PEP 703, `Py_GIL_DISABLED`) build. Probed each call so that
	/// `!pysetautointerpreterstate` swapping the loaded module is reflected
	/// immediately, without a stale cache silently mislabelling the build.
	PYEXT_PUBLIC auto isFreeThreaded() -> bool;

	/// Emits a one-line note to `g_Ext` when a free-threaded build is loaded,
	/// warning that refcounts, managed dicts, frame `localsplus`, and
	/// generator/coroutine state may be inaccurate. No-op on a GIL build.
	/// Intended to be called at the top of user-facing commands.
	PYEXT_PUBLIC auto warnIfFreeThreaded() -> void;

}
