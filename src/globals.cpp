#include "globals.h"
#include "extension.h"
#include "pyextpublic.h"
using namespace PyExt;

#include <engextcpp.hpp>

// Instantiate EngExtCpp's globals.
// This must appear in only one translation unit.
EXT_DECLARE_GLOBALS();

namespace PyExt {

	/// Initializes the extension as calls from DbgEng.dll would.
	/// Intended to be called by the test harness to set up the global state.
	PYEXT_PUBLIC void InitializePyExtForTest(IDebugClient* pClient)
	{
		g_Ext = g_ExtInstancePtr;
		g_Ext->Query(pClient);
	}


	/// Intended to be called by the test harness to reset the global state.
	PYEXT_PUBLIC void UninitializePyExtForTest()
	{
		g_Ext->Release();
	}

}