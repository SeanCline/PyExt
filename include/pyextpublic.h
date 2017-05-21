#pragma once

// Export symbols when building pyext.dll. Import linking against pyext.dll
#ifdef PYEXT_DLL
#	define PYEXT_PUBLIC __declspec(dllexport)
#else
#	define PYEXT_PUBLIC __declspec(dllimport)
#endif
