# Investigation: moving off `engextcpp`

A look at whether PyExt can drop the EngExtCpp framework (`engextcpp.hpp`) for
something thinner, with `std::optional`-based error handling and easier log
redirection. Findings, options with costs, and a recommended path.

> **Decision (2026-05-31): full replacement — Option 3.** engextcpp is removed
> entirely, including the command/arg-parsing layer. The data layer moves to raw
> DbgEng with `std::optional` semantics; the extension ABI
> (`DebugExtensionInitialize` + per-command exports), arg parsing, KnownStruct
> handling, and output are all reimplemented thin and engextcpp-free. The
> analysis below (§1–§5) still stands and explains *why* the work splits the way
> it does; §6 records the decision; §7 is the command/ABI layer Option 3 adds;
> §8 is the phased roadmap; §9 is the spike that kicks it off.
>
> The earlier recommendation (facade-only) is preserved in §6 as the rejected
> alternative — we are knowingly taking on more work to get a genuinely thin,
> dependency-free extension.

---

## 1. What `engextcpp` actually does for us

EngExtCpp is two separable layers. The distinction is the whole investigation.

### (A) Typed remote data access — `ExtRemoteTyped` & friends
Reading the debuggee's memory through the symbol/type system: `Field`,
`HasField`, `GetPtr`, `GetTypeSize`, `Dereference`, `ArrayElement`,
`ReadBuffer`, `GetStdString`, the integer getters, plus expression-driven
construction (`ExtRemoteTyped("(PyDictObject**)@$extin", addr)`).

**This is the layer the requirements are about** — types, typed fields, raw
bytes, optional-on-missing. It maps cleanly onto raw DbgEng:
`IDebugSymbols::GetTypeId` / `GetFieldOffset` / `GetTypeSize` +
`IDebugDataSpaces::ReadVirtual`.

### (B) Extension lifecycle / ABI — `ExtExtension` & macros
The WinDbg plumbing: `EXT_CLASS : ExtExtension`, `EXT_COMMAND` /
`EXT_COMMAND_METHOD`, arg-spec parsing (`"{;s;PyObject address}"`,
`GetUnnamedArgStr`, `HasArg`), `EXT_DECLARE_GLOBALS`, the DLL export wiring,
`m_KnownStructs` / `ExtKnownStruct` handlers, `EvalExprU64`, `FindFirstModule`,
and the `g_Ext` singleton.

**No stated requirement touches layer B.** Replacing it means reimplementing
the WinDbg extension entry-point ABI (`DebugExtensionInitialize`, the
`!command` dispatch table, the `{...}` arg grammar). That is a large, high-risk
effort with ~zero payoff against "thinner, optional, log redirection."

**Conclusion: replace/refactor (A) if anything; keep (B).**

---

## 2. The number that decides feasibility

The scary figure is ~290 `engextcpp` references across 30 files. But almost all
of those are the *cheap* part — `Field` / `HasField` / `GetPtr` / `GetTypeSize`
/ `Dereference` — which is a 1:1 mechanical map onto `GetFieldOffset` +
`ReadVirtual`. They'd be wrapped once in a new `RemoteValue` type and never
touched individually again.

The genuinely hard part is **expression-driven construction**, because that
leans on DbgEng's C++ expression evaluator. Counting *only* those:

| Site | Expression | Difficulty |
|------|-----------|------------|
| `PyMemberDef.cpp:51` | `(char**)@$extin` → deref → deref → `GetString` | trivial (ptr read) |
| `PyMemberDef.cpp:59` | `(int*)@$extin` → deref | trivial |
| `PyMemberDef.cpp:67` | `(Py_ssize_t*)@$extin` → deref | trivial |
| `PyObject.cpp:54` | `(PyObject**)@$extin` | trivial |
| `PyObject.cpp:94` | `(_dictvalues*)((PyObject*)(@$extin)+1)` | **arithmetic** (`base + sizeof(PyObject)`) |
| `PyObject.cpp:121` | `(PyObject***)@$extin` → deref | trivial |
| `PyObject.cpp:123` | `(PyDictObject**)@$extin` → deref | trivial |
| `PyObject.cpp:155` | `(PyDictObject**)@$extin` | trivial |
| `PyDictObject.cpp:109` | `(PyObject**)@$extin` → deref | trivial |
| `PyTypeObject.cpp:75` | `(void**)@$extin` → deref | trivial |

**10 sites. 9 are "read a pointer-typed value at an address, then maybe deref"**
— i.e. `readPointer(addr)` / `readPointer(readPointer(addr))`. Exactly one
(`PyObject.cpp:94`) does pointer arithmetic, and it's just `base + sizeof(PyObject)`,
which we can compute from `GetTypeSize("PyObject")`.

Two more expression uses live outside the table and are special:
- `ExtHelpers.cpp` `getPointerSize()` → `"sizeof(void*)"` — replace with
  `IDebugControl::IsPointer64Bit()` (already trivially derivable).
- `extension.cpp` `evalOffset()` → `"(void*)(" + userArg + ")"` — this evaluates
  a **user-supplied** expression and genuinely needs the evaluator. Keep it on
  `IDebugControl::Evaluate`; it's a command-layer concern (B), not a data read.

So the "you can't replace the expression evaluator" objection collapses: we
don't use it for anything load-bearing except one user-facing eval that stays.

**Toolchain note:** the project compiles with `stdcpplatest` (C++23), so the
optional API can use `optional::and_then` / `transform` / `or_else` for clean
chained reads — see §4.

---

## 3. The three options, with honest costs

### Option 1 — Facade over engextcpp (engextcpp stays underneath)
`RemoteType.h` is already the seam (it privately holds a
`shared_ptr<ExtRemoteTyped>`). Add a thin typed-read API on top that:
- wraps throwing calls in try/catch and returns `std::optional` (§4),
- routes all output through an injectable sink (§5).

- **Meets all four requirements:** ✅ types, ✅ typed fields, ✅ raw bytes (already
  there via `ReadBuffer`), ✅ optional-on-missing, ✅ log redirection.
- **Cost:** ~1–3 days. New header + light edits at call sites that want optional
  semantics. Zero ABI risk. engextcpp dependency unchanged.
- **Downside:** not "thinner" in the dependency sense — engextcpp is still
  linked. Doesn't shrink build or remove the `.obj`.

### Option 2 — Replace the data layer (A) with raw DbgEng; keep (B)
New `RemoteValue` class backed by `IDebugSymbols5` + `IDebugDataSpaces4`,
injected with the `IDebugClient` (no `g_Ext` singleton in the data path).
Refactor the 10 expression sites (§2) into explicit offset arithmetic + typed
reads. Keep `ExtExtension`, the command macros, arg parsing, KnownStruct
handlers.

- **Meets all four requirements** and is genuinely thinner — the data path no
  longer touches engextcpp; it becomes pure DbgEng COM.
- **Cost:** ~1.5–3 weeks including parity testing. The work is concentrated in
  `RemoteType.cpp`, `ExtHelpers.h` (`readIntegral`/`readArray`), and the 10
  expression sites. Everything else recompiles against the new `RemoteValue`.
- **Upside:** more testable — injecting `IDebugClient` kills the singleton in
  the data layer, so reads can be unit-tested without `InitializeGlobalsForTest`.
- **Downside:** real reimplementation risk. The 756-assertion dump suite is the
  parity oracle; budget for chasing subtle type-size / sign-extension diffs.

### Option 3 — Full replacement including the command ABI
Everything in Option 2 **plus** reimplementing `DebugExtensionInitialize`, the
`!command` dispatch, the `{;s;...}` arg grammar, and KnownStruct registration
on raw DbgEng.

- **Cost:** weeks more, for the layer no requirement asked about.
- **Recommendation: don't.** No marginal benefit against the stated goals; pure
  risk. Listed only to be explicit that "move away from engextcpp" *literally*
  would include this, and we're choosing not to.

---

## 4. The `std::optional` contract — the spine of the design

"Return optional instead of throwing" is underspecified today: the codebase
already mixes three strategies — `fieldAsPyObject` returns `{}`,
`utils::ignoreExtensionError` swallows via `ExtCaptureOutputA`, and
`readIntegral` / `readArray` **throw** `ThrowRemote`. Any new API has to pick a
deliberate contract. Proposed:

| Failure | Today | Proposed |
|---------|-------|----------|
| Field name absent from type | throw / `HasField` guard | **`nullopt`** |
| Pointer/field value is 0 (null) | caller checks `GetPtr()==0` | **`nullopt`** (distinguish from "read failed" only if a caller needs it) |
| Address not in dump (`ReadVirtual` fails) | throw | **throw** — this is a real, actionable error (incomplete dump); silently dropping it hid bugs before (see `plan.md` item 1) |
| Symbol module not loaded | throw | **throw** — surfaced once at `ensureSymbolsLoaded`, not per-read |
| Type-size vs requested-width mismatch | throw | **throw** — programmer error, not data condition |

Rule of thumb: **expected-absent → `nullopt`; environment-broken → throw.**
This keeps "missing field on this Python version" ergonomic (the common case,
and what the user asked for) while not masking truncated dumps — the exact
silent-data-loss trap called out in `plan.md`.

Sketch (C++23 chaining):

```cpp
class RemoteValue {
public:
    // expected-absent semantics
    auto field(std::string_view name) const -> std::optional<RemoteValue>;
    auto tryField(std::initializer_list<std::string_view> names) const
        -> std::optional<RemoteValue>;   // version-rename fallback (plan.md item 3)

    template <std::integral T> auto as() const -> T;          // throws on env failure
    auto ptr()  const -> std::uint64_t;                       // throws on env failure
    auto bytes(std::size_t n) const -> std::vector<std::byte>; // raw access

    auto deref() const -> RemoteValue;
};

// chained, no exceptions for the missing-field path:
auto line = obj.field("ob_base")
               .and_then([](auto v){ return v.field("co_firstlineno"); })
               .transform([](auto v){ return v.as<int>(); });   // optional<int>
```

This single contract also subsumes `plan.md` item 3's `tryField` helper.

---

## 5. Log / output redirection

Today output is hard-wired to `ExtExtension`'s `Out` / `Dml` / `Warn` / `Err`
and captured (when suppressing noise) via `ExtCaptureOutputA` — see
`extension.cpp` `printDml`/`ensureSymbolsLoaded` and
`utils::ignoreExtensionError`. Redirection means subclassing capture objects,
which is awkward.

Thin fix, independent of Options 1/2: introduce an output sink interface and
route everything through it.

```cpp
struct OutputSink {
    virtual void out (std::string_view) = 0;
    virtual void dml (std::string_view) = 0;
    virtual void warn(std::string_view) = 0;
    virtual void err (std::string_view) = 0;
    virtual ~OutputSink() = default;
};
```

- **Default sink** forwards to `g_Ext->Out/Dml/...` — behavior unchanged.
- **Capture sink** replaces `ExtCaptureOutputA` for the "ignore this output"
  cases — no DbgEng capture object, just a buffer.
- **Test sink** collects output into a string for assertions — removes the
  current need to scrape WinDbg output in tests.

Pass the sink into the command handlers / `RemoteType` rather than reaching for
`g_Ext`. This is the cleanest standalone win and is worth doing **regardless of
which option** is chosen for the data layer.

---

## 6. Decision

**Chosen: Option 3 — full replacement, including the command/arg layer.**
engextcpp is removed in its entirety. We accept the extra cost of reimplementing
layer (B) to get a single, thin, owned abstraction with no external framework
dependency, uniform `std::optional` error handling, and first-class output
redirection.

### Rejected alternatives (recorded for posterity)
- **Option 1 (facade over engextcpp).** Meets the four literal requirements at
  ~days of cost, but leaves engextcpp linked — not "thinner" in the dependency
  sense, which is an explicit goal here. Rejected.
- **Option 2 (replace data layer only, keep command ABI).** A strict subset of
  Option 3; its work is Phases 1–3 below. We're continuing through Phases 4–6
  rather than stopping. Not rejected so much as *subsumed*.

### What "full" buys, and what it costs
- **Buys:** zero `engextcpp.hpp` includes; the `.def` exports are our own
  functions; arg parsing is a ~100-line parser we control; output goes through
  an injectable sink; the data path is unit-testable without the `g_Ext`
  singleton.
- **Costs:** we now own the WinDbg extension ABI (§7) and must keep it correct
  across DbgEng versions. This is well-documented and stable, but it's surface
  we didn't previously maintain. Budget ~3–5 weeks total incl. parity testing.

---

## 7. The command / arg / ABI layer (what Option 3 adds over Option 2)

This is the layer engextcpp's macros generate for us today. Replacing it is
mechanical but must match the DbgEng extension contract exactly. The current
`src/PyExt.def` already names every export WinDbg expects — that list *is* the
contract:

```
DebugExtensionInitialize  DebugExtensionUninitialize  DebugExtensionNotify
KnownStructOutputEx  help  pyobj  pystack  pysymfix  pysetautointerpreterstate
pyinterpreterframe
```

### 7.1 Extension entry points
Hand-write the three lifecycle exports (DbgEng calls these by name):

```cpp
extern "C" HRESULT CALLBACK DebugExtensionInitialize(PULONG version, PULONG flags);
extern "C" void    CALLBACK DebugExtensionUninitialize(void);
extern "C" HRESULT CALLBACK DebugExtensionNotify(ULONG notify, ULONG64 arg);
```

`DebugExtensionInitialize` sets `*version = DEBUG_EXTENSION_VERSION(1,0)` and
`*flags = 0`. No per-call client is handed here; each *command* receives the
client instead (next).

### 7.2 Command exports & dispatch
Each `!name` is an exported C function with the fixed DbgEng signature:

```cpp
extern "C" HRESULT CALLBACK pyobj(IDebugClient* client, PCSTR args);
```

These become one-line thunks into a registry, so command bodies stay clean:

```cpp
extern "C" HRESULT CALLBACK pyobj(IDebugClient* client, PCSTR args)
{ return CommandRegistry::dispatch("pyobj", client, args); }
```

`dispatch` builds a `DbgEngContext` from `client` (QI for Control/Symbols/
DataSpaces/SystemObjects), constructs the `ArgParser`, runs the handler, and
maps thrown exceptions to `HRESULT` + an `Err(...)` line — replicating
engextcpp's catch-and-report so a command can still `throw` for environment
failures (§4).

### 7.3 Arg parsing (replacing the `{;s;...}` grammar)
DbgEng passes the raw post-command text as a single `PCSTR`. PyExt's real needs
are tiny: positional expression/address args and boolean flags (`-all`). A
~100-line `ArgParser` covers it:

```cpp
class ArgParser {
public:
    ArgParser(std::string_view raw);
    auto flag(std::string_view name) const -> bool;          // HasArg("all")
    auto positional(size_t i) const -> std::optional<std::string_view>; // GetUnnamedArgStr(0)
    auto count() const -> size_t;                            // m_NumUnnamedArgs
};
```

We do **not** reproduce engextcpp's typed arg-spec DSL — commands evaluate their
own expression args via the context's `evaluate()` (which wraps
`IDebugControl::Evaluate`, the one place the DbgEng expression evaluator is still
needed; see §2). `help` is generated from the registry's command descriptions.

### 7.4 KnownStruct pretty-printing
engextcpp's `m_KnownStructs` + `ExtKnownStruct` becomes the raw
`KnownStructOutputEx` export. On `DEBUG_KNOWN_STRUCT_GET_NAMES` we return the
NUL-separated type-name list (`PyVarObject`, `PyObject`, `_object`, … — exactly
the array in `extension.cpp`); on `DEBUG_KNOWN_STRUCT_GET_SINGLE_LINE_OUTPUT` we
fill the caller's buffer with the one-line repr. This is the fiddliest part of
the ABI (buffer-size protocol) and is a named spike risk (§9).

### 7.5 Output
All `Out`/`Dml`/`Warn`/`Err`/`AppendString` usage routes through the `OutputSink`
(§5), whose default implementation calls `IDebugControl::ControlledOutput` /
`Output` / `OutputDml`. `ExtCaptureOutputA` (used by `ensureSymbolsLoaded` and
`utils::ignoreExtensionError`) is replaced by swapping in a `CaptureSink` — no
DbgEng output-capture object needed.

### 7.6 Misc engextcpp helpers to port
- `EvalExprU64` → `IDebugControl::Evaluate(..., DEBUG_VALUE_INT64)`.
- `FindFirstModule("python???")` → `IDebugSymbols::GetModuleByName` /
  `GetModuleNames` loop (already used directly in `pysymfix.cpp`).
- `ExtBuffer<char>` → `std::string` / `std::vector<char>`.
- `m_Symbols->Reload` etc. are already raw `IDebugSymbols` calls — unchanged.

---

## 8. Phased roadmap

Each phase keeps the suite green (`test/PyExtTest/`, 756 assertions) before the
next begins. Phases 1–3 are the Option-2 data layer; 4–6 are the Option-3
command/ABI layer.

| Phase | Scope | Gate |
|-------|-------|------|
| **0** | Spike (§9): prove ABI shape + hardest data read in isolation | go/no-go |
| **1** | `DbgEngContext` + `OutputSink`; route all output through the sink (engextcpp still present) | suite green, output unchanged |
| **2** | `RemoteValue` data layer over raw DbgEng; port `readIntegral`/`readArray`; reimplement the 10 expression sites (§2). `RemoteType` delegates to `RemoteValue`. | suite green incl. managedDict parity |
| **3** | Flip the `std::optional` contract (§4); replace throwing field reads with optional where "expected-absent" | suite green; new optional unit tests |
| **4** | `CommandRegistry` + `ArgParser`; command bodies stop using `ExtExtension`/`EXT_COMMAND` | each `!command` works under live WinDbg |
| **5** | Hand-write `DebugExtension*` exports + `KnownStructOutputEx`; drop `EXT_DECLARE_GLOBALS` / `g_Ext` | extension loads, `!pyobj`/`!pystack` work, known-struct printing works |
| **6** | Remove every `#include <engextcpp.hpp>` (incl. `pch.h`), drop the lib from the vcxproj, delete `globals.cpp` engextcpp bits | grep for `engextcpp`/`ExtRemoteTyped`/`g_Ext` returns 0; suite green; manual WinDbg smoke test |

**Test harness migration (spans Phases 1–5):** `InitializeGlobalsForTest`/
`UninitializeGlobalsForTest` currently set up `g_Ext` and `Query(client)`.
Replace with constructing a `DbgEngContext` from the harness's `IDebugClient*`
and a `TestSink` for output assertions. This *removes* the singleton from tests
— a net testability win.

---

## 9. The spike (Phase 0) — what this branch proves

`spike/drop-engextcpp`. Two independent go/no-go proofs; both must pass before
Phase 1 starts. Each proves a *hardest* case, not an easy one.

### Proof A — bare extension ABI (validates layer B is reproducible)
A minimal `.dll` with **no engextcpp**, exporting hand-written
`DebugExtensionInitialize` and a single `!pyspike` command that QIs the client
for `IDebugControl` and prints one line via `ControlledOutput`. Acceptance:
WinDbg `.load`s it and `!pyspike` prints. This retires the biggest unknown of
Option 3 — that we can satisfy the export ABI without the framework.
*Stretch:* add `KnownStructOutputEx` returning the name list + a one-liner, and
confirm `dt` triggers it (§7.4 is the fiddliest ABI corner).

### Proof B — hardest data read on raw DbgEng (validates layer A)
Reimplement `PyObject::managedDict()`'s expression reads — the arithmetic site
`(_dictvalues*)((PyObject*)(@$extin)+1)` at `PyObject.cpp:94` **plus** the
`(PyObject***)@$extin`→deref chain — using only `IDebugSymbols::GetFieldOffset`/
`GetTypeSize` + `IDebugDataSpaces::ReadVirtual` + manual offset arithmetic
(`base + GetTypeSize("PyObject")`). Acceptance: **byte-for-byte parity** with the
engextcpp result against `object_details.dmp` / `object_types.dmp` in the
existing suite. Proving `refCount` (a plain `Field` read) proves nothing — the
cast-expression path is the real risk.

### If the spike fails
- Proof A fails (ABI quirk we can't reproduce) → fall back to **Option 2**: keep
  engextcpp's command/ABI layer, replace only the data layer. Requirements still
  met; "thinner" is partial.
- Proof B fails (an expression site relies on evaluator behavior we can't
  replicate) → keep that *specific* read on `IDebugControl::Evaluate` and isolate
  it; the rest of the data layer still migrates.

---

## 10. Progress log

### Phase 1 — increment 1 (2026-05-31): output sink landed, `pyobj` converted
- `src/dbg/OutputSink.{h,cpp}` implemented as a **printf-passthrough** over
  `IDebugControl::OutputVaList`/`ControlledOutputVaList`, mirroring engextcpp's
  `ExtExtension::Out/Warn/Err/Dml` channel+mask mapping verbatim (NOT a
  `std::format` wrapper — that would break `%y` symbol resolution, `%p`, and DML
  `<link>` markup). Wired into `PyExt.vcxproj` with `PrecompiledHeader=NotUsing`.
- `extension.cpp`: `pyobj` and the shared `printDml` now print through a
  `ControlOutputSink(m_Control)` instead of inherited `Out`/`Dml`.
- **Verification — output is the gate, not the suite.** The 756-assertion suite
  drives the *Remote object model*, not the `EXT_COMMAND` bodies, so it cannot
  detect output changes. Used a golden-master cdb diff instead:
  `cdb -z object_details.dmp -y <sym> -c ".load pyext.dll; !pyobj python314!PyType_Type; !pyobj python314!_Py_NoneStruct; q"`
  and `!pystack -all` on `pystack_all_test.dmp`, before vs after. **Result:
  byte-identical** (only diff was cdb's own nondeterministic startup-timing
  line). `%y` symbol+offset output reproduced exactly.
- Next increments: convert `pystack.cpp`, `pysymfix.cpp`,
  `pysetautointerpreterstate.cpp`, then replace `ExtCaptureOutputA` suppression
  in `ensureSymbolsLoaded`/`ignoreExtensionError` with `NullSink` (keep on the
  same engine until the capture mechanism itself is replaced).

### ⚠ Discovered pre-existing issue (NOT caused by this migration)
While establishing the suite baseline, found that `ObjectTypesTest.cpp:262`
(`REQUIRE(list_obj.numItems() == 3)`) **fails and then crashes** (`0xC0000005`)
nondeterministically against the Python 3.14 dumps — assertion counts vary
run-to-run (756 once, then 713/706/702/124-alone). Confirmed independent of this
work: reproduces on clean `spike/drop-engextcpp` HEAD with all Phase-1 changes
stashed, with the ObjectTypes case run in isolation, and with local-cache-only
symbols (rules out network/symbol flakiness). Smells like UB (heap corruption /
uninitialized read) in the list/locals read path on the 3.14 layout — adjacent
to the correctness items in `plan.md`. **Tracked separately; it gates a clean
"suite green" signal but does not block engextcpp Phase-1 increments, which are
verified by golden-master diff + A/B suite parity (clean-HEAD and this branch
crash identically).**

## 11. Migration touch-points (quick reference)

- **Data seam:** `include/RemoteType.h` already hides `ExtRemoteTyped` behind a
  `shared_ptr` — `RemoteValue` slots in here (Phase 2).
- **Width dispatch:** `src/ExtHelpers.h` `readIntegral`/`readArray` — port once.
- **Expression sites:** the 10 in §2 (PyMemberDef ×3, PyObject ×5, PyDictObject
  ×1, PyTypeObject ×1) — entire hard surface of layer A.
- **ABI surface:** `src/PyExt.def` (export names), `extension.cpp` (command
  bodies + KnownStruct array + `evalOffset`), `globals.cpp`
  (`EXT_DECLARE_GLOBALS`), `pch.h` (the engextcpp include).
- **Parity oracle:** `test/PyExtTest/` dump suite. Harness already hands a raw
  `IDebugClient*` to `InitializeGlobalsForTest`.
- **Not foreign territory:** `globals.h` includes `DbgEng.h`; `pysymfix.cpp`
  already drives `IDebugSymbols` directly.
