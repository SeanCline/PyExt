# Spike: drop engextcpp (`spike/drop-engextcpp`)

Phase 0 of `engextcpp-replacement.md`. Two go/no-go proofs. Both must pass
before Phase 1. **Nothing here is built or verified yet** — this is scaffolding
that defines the proofs and the new layer's contracts.

## Proof A — bare extension ABI (no engextcpp)
**File:** `src/spike/SpikeExports.cpp`
**Goal:** show we can satisfy the WinDbg export ABI without the framework.

- [ ] Build `SpikeExports.cpp` into a tiny `.dll` (own minimal project/cmd, or a
      temporary `pyspike` config) exporting `DebugExtensionInitialize` and one
      `!pyspike` command. No `engextcpp.hpp` include.
- [ ] In WinDbg/CDB: `.load pyspike.dll`, run `!pyspike hello`.
- [ ] **Accept:** `pyspike: bare-ABI extension alive. args=[hello]` prints.
- [ ] *Stretch:* add `KnownStructOutputEx` (names + one-liner) and confirm `dt`
      triggers it (the fiddliest ABI corner — buffer-size protocol).

## Proof B — hardest data read on raw DbgEng
**Target:** `PyObject::managedDict()` expression reads — `PyObject.cpp:94`
`(_dictvalues*)((PyObject*)(@$extin)+1)` (arithmetic) **plus** the
`(PyObject***)@$extin`→deref chain.

- [ ] Implement the read using only `IDebugSymbols::GetFieldOffset`/`GetTypeSize`
      + `IDebugDataSpaces::ReadVirtual` + manual offset arithmetic
      (`base + GetTypeSize("PyObject")`). See `RemoteValue` (`src/dbg/`).
- [ ] Run it alongside the existing engextcpp path against `object_details.dmp`
      and `object_types.dmp`.
- [ ] **Accept:** byte-for-byte parity on the returned dict pointer / values
      pointer. (Proving `refCount` proves nothing — plain `Field` reads are the
      easy case.)

## Fallbacks if a proof fails
- **A fails** → Option 2: keep engextcpp's command/ABI layer, replace only the
  data layer. Requirements still met; "thinner" becomes partial.
- **B fails** → keep that *one* expression site on `IDebugControl::Evaluate`,
  isolate it, migrate the rest.

## Scaffold laid down on this branch
- `src/spike/SpikeExports.cpp` — Proof A skeleton (self-contained).
- `src/dbg/OutputSink.h` — output redirection interface (§5 of the plan).
- `src/dbg/DbgEngContext.h` — replaces the `g_Ext` singleton; holds the COM
  interfaces, injectable for tests.
- `src/dbg/RemoteValue.h` — the `std::optional` typed-read API (§4 contract).

These compile-check independently of the existing tree by design; wiring them
into the build is Phase 1, not the spike. Do **not** start editing the 30
existing files until both proofs are green.
