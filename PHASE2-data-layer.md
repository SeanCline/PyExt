# Spike next phase — Phase 2: replace the `ExtRemoteTyped` data layer

Companion to `engextcpp-replacement.md` (the decision + full roadmap) and
`SPIKE.md` (Phase 0 proofs). Phase 1 increment 1 (the `OutputSink`) is landed and
verified; this document is the detailed plan for **Phase 2 — swapping the typed
remote-data layer off `ExtRemoteTyped` onto the raw-DbgEng `RemoteValue`** defined
in `src/dbg/RemoteValue.h`.

This is the largest single phase and the one the whole migration hinges on. It is
sequenced so the 756-assertion suite (and the golden-master diff) stays green
after every step.

> Prereq carried in from Phase 0: the `@$extin` cast-expression sites lower to
> `address + GetTypeSize/GetFieldOffset` + `ReadVirtual` (proven against the 3.14
> dump). Phase 2 is the mechanical execution of that, behind the existing
> `RemoteType` seam.

> 🚨 **HEISENBUG STANDING ORDER.** A pre-existing, resource-pressure-gated,
> optimization-sensitive crash lives in the `ObjectTypesTest` `list_obj` path on
> the 3.14 dumps (Debug + ASan mask it; reproduces only in plain Release under
> memory pressure / certain section orders — measured 5/5 crash under load,
> 0/10 quiet). It is **not** caused by the engextcpp migration (clean HEAD
> reproduces it). **If it reappears during any Phase 2 work: STOP. Drop
> everything and debug it live while it is still failing** — capture a failing
> `--rng-seed`, attach cdb, `kP` the AV, then `#pragma optimize("",off)`
> per-TU bisection (`PyDictKeysObject.cpp` → `PyDictObject.cpp` → `PyObject.cpp`
> → `PyTypeObject.cpp`). A live repro is the only path to root cause; do not let
> it slip back to quiescent. Suspected class: intermittent/unchecked DbgEng read
> yielding garbage `ob_size`.

---

## 0. Finish Phase 1 first (small, unblocks clean output routing)

Before the data layer, close out output routing so no command still reaches for
`g_Ext`/`ExtExtension::Out`:

1. Convert `pystack.cpp`, `pysymfix.cpp`, `pysetautointerpreterstate.cpp`,
   `pyinterpreterframe` to `ControlOutputSink` (pattern already set by `pyobj`).
   Verify each with a golden-master cdb diff (`!pystack`, `!pystack -all`,
   `!pysymfix`, `!pysetautointerpreterstate`).
2. Replace `ExtCaptureOutputA` suppression in `ensureSymbolsLoaded` and
   `utils::ignoreExtensionError` with a `NullSink` — **but keep these on the same
   DbgEng output engine until the capture mechanism itself is gone**, or
   suppression silently stops working (stray symbol-error text leaks). Verify the
   "symbols missing" banner still appears/suppresses correctly.

Gate: golden-master diffs byte-identical for every command; suite unchanged.

---

## 1. Build out `DbgEngContext` (the `g_Ext` replacement for the data path)

`src/dbg/DbgEngContext.h` exists as scaffold. Implement `DbgEngContext.cpp`:

- Ctor QIs the `IDebugClient*` for `IDebugControl`, `IDebugSymbols3`,
  `IDebugDataSpaces4`, `IDebugSystemObjects`; throws `RemoteReadError` on failure.
- `evaluateU64(expr)` → `IDebugControl::Evaluate(..., DEBUG_VALUE_INT64)`,
  `nullopt` on failure (replaces the `try/catch(ExtException)` in `evalOffset`).
- `pointerSize()` → `IDebugControl::IsPointer64Bit()` (replaces the
  `"sizeof(void*)"` eval trick in `utils::getPointerSize`).
- `qualifyPythonSymbol(sym)` → `IDebugSymbols::GetModuleByName("python*")` +
  `GetModuleNames` (replaces `FindFirstModule("python???")` in
  `getFullSymbolName`).

Wire it into the test harness: have `InitializeGlobalsForTest` construct a
`DbgEngContext` from the harness `IDebugClient*` alongside (not replacing yet)
`g_Ext`. This is the injection point that removes the singleton from the data
layer and makes reads unit-testable.

Gate: a focused unit test (mirrors the Phase-0 spike) opens `object_details.dmp`,
builds a `DbgEngContext`, asserts `pointerSize()==8`,
`evaluateU64("python314!PyType_Type")` resolves, `qualifyPythonSymbol("ob_type")`
returns `python314!ob_type`.

---

## 2. Implement `RemoteValue` over raw DbgEng

Implement `src/dbg/RemoteValue.cpp` against `IDebugSymbols3` + `IDebugDataSpaces4`.
Method-by-method mapping from `ExtRemoteTyped`:

| `RemoteValue` | DbgEng calls | Replaces |
|---|---|---|
| ctor(typeName, addr) | `GetSymbolTypeId`/`GetTypeId` | `ExtRemoteTyped(name, addr, true)` |
| `hasField(name)` | `GetFieldTypeAndOffset` → found? | `HasField` |
| `field(name)` | `GetFieldOffset` + new `RemoteValue` at addr+off, field's type | `Field` (→ `optional`) |
| `tryField({...})` | first `hasField` hit | scattered `HasField` chains |
| `baseField(name)` | walk `ob_base` (port `PyObject::baseField`) | `PyObject::baseField` |
| `deref()` | read ptr (below) → `RemoteValue` at target, pointee type | `Dereference` (→ `optional` on null) |
| `element(i)` | addr + i*`GetTypeSize(elemType)` | `ArrayElement` |
| `as<T>()` | `ReadVirtual` `GetTypeSize` bytes, sign-extend per width | `utils::readIntegral` |
| `ptr()` | `ReadVirtual` pointerSize bytes | `GetPtr` |
| `str()` | `ReadMultiByte`/loop `ReadVirtual` to NUL | `GetString`/`GetStdString` |
| `bytes(n)` / `array<T>(n)` | `ReadVirtual(n)` | `utils::readArray` + `ReadBuffer` |
| `typeSize()` | `GetTypeSize` | `GetTypeSize` |

**Error contract (the spine — engextcpp-replacement.md §4):**
- field-not-present, null-pointer-deref → `nullopt`.
- `ReadVirtual` short read / unmapped address, unresolved type/symbol, width
  mismatch → throw `RemoteReadError`. (Do **not** silently fall back — that hid
  bugs before; see `plan.md`.)

Use C++23 `optional::and_then`/`transform` so chained reads stay exception-free on
the expected-absent path.

Gate: standalone unit tests for `RemoteValue` against `object_details.dmp`: read
`refCount`, `ob_type`, walk `ob_base`, read an array. Compare each to the
`ExtRemoteTyped` answer (both still linked) — must match.

---

## 3. Route `RemoteType` through `RemoteValue`

`include/RemoteType.h` already hides `ExtRemoteTyped` behind a `shared_ptr`. Add a
parallel `RemoteValue` member and switch `offset()`/`symbolName()`/`remoteType()`
consumers over incrementally. Keep `ExtRemoteTyped` available during the
transition so a half-migrated tree still builds.

Port the width-dispatch helpers `utils::readIntegral`/`readArray`/
`readOffsetArray` to `RemoteValue` equivalents (they are the only integer/array
size logic; port once, delete the `ExtRemoteTyped` versions at the end).

---

## 4. Reimplement the 10 cast-expression sites (the hard surface)

These are the only `@$extin` sites (enumerated in `engextcpp-replacement.md` §2).
Convert each to explicit `RemoteValue` offset arithmetic + read:

- `PyObject.cpp` ×5 — `managedDict`/`dict`/`slots`. The arithmetic site
  `(_dictvalues*)((PyObject*)(@$extin)+1)` → `base + RemoteValue("PyObject").typeSize()`
  (proven in Phase 0). The `(T**)@$extin`→deref chain → `RemoteValue(ptrType, addr).deref()`.
- `PyMemberDef.cpp` ×3, `PyDictObject.cpp` ×1, `PyTypeObject.cpp` ×1 — all the
  trivial `(T*)@$extin`→deref pattern.

Gate: this is the section the Phase-0 Proof-B parity check targeted. Assert
**end-to-end byte-parity of `managedDict()`** on `manDictRes` (the open Proof-B
item) — `ObjectDetailsTest` is the oracle. Then the full suite.

> ⚠ Stabilize the suite first. There is a pre-existing, order-/optimization-
> sensitive crash in `ObjectTypesTest` (`list_obj` path) on the 3.14 dumps
> (Debug + ASan mask it; reproduces in plain Release under certain section
> orders). It is being fixed on `master` separately. Phase 2's parity gate is
> unreliable until that lands — track it as a hard dependency of step 4.

---

## 5. Drop `ExtRemoteTyped` from the data layer

When every reader is on `RemoteValue`:
- Remove the `ExtRemoteTyped` member from `RemoteType`.
- Delete the `ExtRemoteTyped` integer/array helpers.
- `grep -r ExtRemoteTyped src/` must be empty (the command/ABI layer is Phase
  4–6; data layer is done).

Gate: full suite green; golden-master diffs unchanged; `RemoteValue` unit tests
green. This completes Phase 2 — the data path is engextcpp-free; only the command
/ABI layer (`ExtExtension`, `EXT_COMMAND`, KnownStruct, exports) still depends on
the framework, to be removed in Phases 4–6.

---

## Verification toolkit (carried from Phase 0/1)

- **Build/test loop:** `msbuild PyExt.sln /p:Configuration=Release /p:Platform=x64`
  (~3–11s incremental), then `PyExtTest.exe --symbol-path cache*C:\symbols` from
  `test/scripts`. Local-cache-only symbols (`cache*C:\symbols`) are deterministic.
- **Output parity:** golden-master cdb diff — `.load pyext.dll; !pyobj python314!PyType_Type; !pyobj python314!_Py_NoneStruct; !pystack -all; q`, before vs after.
- **Data parity:** keep `ExtRemoteTyped` linked through steps 2–4 so each
  `RemoteValue` read can be diffed against the framework's answer on a live dump.
- **Heisenbugs:** Debug and ASan can mask optimization-sensitive UB; use
  plain-Release + fixed `--rng-seed` + `#pragma optimize("",off)` per-TU bisection.
