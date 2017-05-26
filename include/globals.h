#pragma once

#include "pyextpublic.h"
#include <DbgEng.h>

namespace PyExt {

	// These functions are provided for testability.
	// They allow the test harness to initialize/uninitialize the global extension instance
	// as would typically happen when calling an extension command.
	PYEXT_PUBLIC auto InitializeGlobalsForTest(IDebugClient* pClient) -> void;
	PYEXT_PUBLIC auto UninitializeGlobalsForTest() -> void;

}