# Spike: drop engextcpp (`spike/drop-engextcpp`)

Phase 0 of `engextcpp-replacement.md`. Two go/no-go proofs.

**Status (2026-05-31): both proofs green — Option 3 is GO.** Proof A executed
end-to-end in `cdb` (✅). Proof B's core risk — that hand-lowered arithmetic
reproduces the DbgEng evaluator — validated against the real Python 3.14 dump
(☑); the remaining end-to-end parity harness is Phase-2 work, not a spike
blocker. Phase 1 may begin.

## Proof A — bare extension ABI (no engextcpp) ✅ PASSED (2026-05-31)
**File:** `src/spike/SpikeExports.cpp`
**Goal:** show we can satisfy the WinDbg export ABI without the framework.

- [x] Build `SpikeExports.cpp` into a tiny `.dll` (own minimal project/cmd, or a
      temporary `pyspike` config) exporting `DebugExtensionInitialize` and one
      `!pyspike` command. No `engextcpp.hpp` include.
- [x] In WinDbg/CDB: `.load pyspike.dll`, run `!pyspike hello`.
- [x] **Accept:** `pyspike: bare-ABI extension alive. args=[hello]` prints.
- [ ] *Stretch:* add `KnownStructOutputEx` (names + one-liner) and confirm `dt`
      triggers it (the fiddliest ABI corner — buffer-size protocol).

**Result.** Built with MSVC 14.51 (`cl /std:c++latest /LD`), exports verified
clean/undecorated (`DebugExtensionInitialize`, `DebugExtensionUninitialize`,
`pyspike`). Loaded in `cdb.exe` (WinKit 10.0.26100 x64):

```
> cdb -c ".load pyspike.dll; !pyspike hello from cdb; .unload pyspike; q" cmd.exe /c exit
pyspike: bare-ABI extension alive. args=[hello from cdb]
Unloading ...\pyspike.dll extension DLL
```

Confirms: `.load` accepts a non-engextcpp DLL; the lifecycle + command exports
dispatch; the command receives the **raw unparsed arg string** (`hello from
cdb`) as a `PCSTR` — exactly the input our own `ArgParser` (§7.3) consumes, so
the `{;s;...}` grammar is genuinely unnecessary. The biggest Option-3 unknown
(owning the export ABI) is retired.

Reproduce: `x64/spike/pyspike.dll` is the build output; rebuild via the cl
command in the commit, or wire `src/spike/` into a vcxproj config.

## Proof B — hardest data read on raw DbgEng ☑ CORE RISK VALIDATED (2026-05-31)
**Target:** `PyObject::managedDict()` expression reads — `PyObject.cpp:94`
`(_dictvalues*)((PyObject*)(@$extin)+1)` (arithmetic) **plus** the
`(PyObject***)@$extin`→deref chain.

The spike's real risk was: *does hand-lowered offset arithmetic
(`GetTypeSize`/`GetFieldOffset` + `ReadVirtual`) reproduce DbgEng's C++
expression evaluator?* — because that evaluator is what `ExtRemoteTyped`'s
`@$extin` casts use. Validated directly in `cdb` against the real **Python
3.14** dump (`object_details.dmp`, `python314` module), anchored on the exported
`PyType_Type`:

| Primitive (the two `managedDict` relies on) | Evaluator (engextcpp) | Hand-lowered (raw) | |
|---|---|---|---|
| `(PyObject*)T + 1` | `0x7ffd``605fada0` | `T + GetTypeSize(PyObject)=0x10` → `0x7ffd``605fada0` | ✅ |
| `((PyObject*)T)->ob_type` | `0x7ffd``605fad90` | `poi(T + GetFieldOffset(ob_type)=0x8)` → `0x7ffd``605fad90` | ✅ |

`sizeof(python314!PyObject) == 0x10`; `offsetof(ob_type) == 0x8`. Both lower
exactly. The evaluator-only capability is therefore *not* load-bearing for the
data layer — the cast-expression sites (§2 of the plan) reduce to
`address + size/offset` + `ReadVirtual`.

- [x] Prove the arithmetic-lowering primitives against the real 3.14 dump.
- [ ] **Remaining (Phase 2, not a spike blocker):** implement `RemoteValue`'s
      DbgEng-backed reads and assert *end-to-end* byte-parity of the whole
      `managedDict()` result on a managed-dict instance (`manDictRes`) — the
      existing `ObjectDetailsTest` is the oracle. This proves the wiring, not the
      mechanism (the mechanism is proven above).

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
