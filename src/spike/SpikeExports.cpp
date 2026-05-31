// Proof A for the engextcpp removal spike (see SPIKE.md).
//
// A minimal WinDbg/DbgEng extension with NO dependency on engextcpp.hpp. It
// demonstrates that we can satisfy the extension export ABI by hand:
//   - DebugExtensionInitialize / DebugExtensionUninitialize (lifecycle)
//   - one !command export with the raw DbgEng signature
//
// Build this into a standalone pyspike.dll (exports come from __declspec below,
// so no .def is needed for the spike), .load it in WinDbg, and run `!pyspike`.
//
// This file intentionally includes ONLY <DbgEng.h> + <windows.h> — if it builds
// and loads, the "can we live without the framework's entry-point machinery?"
// question is answered yes.

#include <windows.h>
#include <DbgEng.h>

// DbgEng calls the lifecycle exports by these exact names. Version is encoded
// with the DEBUG_EXTENSION_VERSION(major, minor) macro from DbgEng.h.
extern "C" __declspec(dllexport) HRESULT CALLBACK
DebugExtensionInitialize(PULONG version, PULONG flags)
{
	*version = DEBUG_EXTENSION_VERSION(1, 0);
	*flags = 0; // no special flags for the spike.
	return S_OK;
}

extern "C" __declspec(dllexport) void CALLBACK
DebugExtensionUninitialize(void)
{
}

// Each `!name` command is an export with this fixed signature. DbgEng hands us
// the active client and the raw, unparsed argument text (everything after the
// command name). In the real replacement this becomes a one-line thunk into
// CommandRegistry::dispatch (see engextcpp-replacement.md §7.2); here we inline
// it to keep the proof self-contained.
extern "C" __declspec(dllexport) HRESULT CALLBACK
pyspike(IDebugClient* client, PCSTR args)
{
	if (client == nullptr)
		return E_INVALIDARG;

	IDebugControl* control = nullptr;
	HRESULT hr = client->QueryInterface(__uuidof(IDebugControl), reinterpret_cast<void**>(&control));
	if (FAILED(hr))
		return hr;

	// This is the primitive every Out()/Warn()/Err() call will ultimately use
	// once OutputSink (src/dbg/OutputSink.h) wraps it.
	control->Output(DEBUG_OUTPUT_NORMAL,
		"pyspike: bare-ABI extension alive. args=[%s]\n",
		(args != nullptr) ? args : "");

	control->Release();
	return S_OK;
}

// Stretch goal (SPIKE.md): implement KnownStructOutputEx here to prove the
// known-struct buffer-size protocol works without ExtKnownStruct. Left as a
// follow-up because it is the fiddliest part of the ABI and is not required to
// answer the core Proof-A question.
//
// extern "C" __declspec(dllexport) HRESULT CALLBACK
// KnownStructOutputEx(PDEBUG_CLIENT client, ULONG flags, ULONG64 address,
//                     PSTR buffer, PULONG bufferChars) { ... }
