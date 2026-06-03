# PyExtTest "weird crash" — investigation notes

Context for the `/d2FH4-` workaround added to `PyExtTest.vcxproj`. The flag is a
**symptom mask, not a fix.** This documents what the crash actually is so the
workaround can be removed once the real bug is addressed.

## Symptom

Optimized (`/O2`) `Release|x64` `PyExtTest.exe` intermittently terminates
abnormally while running the `[object_types]` test case
(`"object_types.py has a stack frame with expected locals."`). Flaky /
load-sensitive; Debug and ASan builds are clean. The original guess was an MSVC
14.51 (VS2026) FrameHandler4 (FH4) exception-unwind miscompile, worked around
with `/d2FH4-` (force the FH3 unwinder).

## It is not a compiler bug

Reproduced under Time Travel Debugging (≈1 crash in 7 runs *under TTD*; bare and
live-debugger runs did not reproduce — the bug is timing-sensitive). Findings
from the trace:

- The faulting process exits with `0xC0000374` (`STATUS_HEAP_CORRUPTION`), **not**
  an access violation, and the **FH4 unwinder appears nowhere on the stack.**
- The two `e06d7363` C++ exceptions on the timeline are the *expected*
  `ExtException` "unable to evaluate 'autoInterpreterState'" throws — normal
  control flow, not a crashing unwind.
- `/d2FH4-` "fixing" it is just codegen/layout perturbation hiding a
  pre-existing memory-corruption heisenbug. Masking ≠ root cause.

So toggling the EH personality is irrelevant to the actual defect.

## Where the corruption is

Crash stack (end of trace):

```
ntdll!RtlFlsSetValue                      <- dies here (FLS callback)
KERNELBASE!FlsSetValue
ucrtbase!...wrapped_invoke
dbghelp!...SymEnumSymbolsExW path
dbgeng!...                                <- enum callback back into dbgeng
pyext!ExtRemoteTyped::ErtIoctl   "Set: unable to evaluate 'autoInterpreterState'"
pyext!ExtRemoteTyped::Set
pyext!make_shared<ExtRemoteTyped>("autoInterpreterState")
pyext!PyExt::Remote::RemoteType::RemoteType
pyext!PyExt::Remote::PyInterpreterState::makeAutoInterpreterState
pyext!PyExt::Extension::ensureSymbolsLoaded
pyext!PyExt::Extension::pysymfix
```

The corruption is detected **inside the dbgeng/dbghelp symbol engine** (its
FLS / symbol-enumeration machinery), reached through `engextcpp`'s
`ExtRemoteTyped`. The trigger is the **repeated *failed* expression evaluations
around `m_Symbols->Reload("/f python*")`** in `ensureSymbolsLoaded` →
`makeAutoInterpreterState`. In the crash run, evaluation fell through to the 3rd
fallback (`"autoInterpreterState"`, a pre-3.7 global that does not exist on
3.14) — i.e. it ran the symbol-reload + failed-eval sequence, not the happy
path. Symbol-load state is what makes it flaky.

## Ruled out (source audit)

The PyExt code on this path is clean RAII: `makeAutoInterpreterState`,
`RemoteType` (thin `make_shared<ExtRemoteTyped>` wrapper), and
`ensureSymbolsLoaded` are well-formed. `engextcpp`'s `ExtCaptureOutput::Output`
buffer growth (`realloc`/`memcpy`) is overflow-guarded and correct, nests
LIFO, and is not on the fault stack. No obvious PyExt-side heap bug found.

Both `dbghelp.dll` and `dbgeng.dll` are the same SDK version
(10.0.26100.8249, copied into the output dir by the PostBuild step) — not a
version mismatch.

## Not yet pinned — and why every capture method fought us

The exact corrupting write is not identified. The bug is a
**TTD-perturbation-sensitive heisenbug**, and the capture techniques are
mutually exclusive with reproducing it:

- **Reproduces only under TTD** (~1 in 7 recorded runs). Bare runs (0/60),
  live under cdb (0/30+), and page-heap-live (0/15+) do **not** reproduce —
  the bug needs TTD's specific timing.
- **Plain TTD clips the fault.** `STATUS_HEAP_CORRUPTION` fires via
  `__fastfail`, which bypasses normal exception dispatch; the trace ends ~2
  instructions before it (last recorded frame: `ntdll!RtlFlsSetValue`, a benign
  instruction). No corrupted-block address to reverse-trace from.
- **`!heap -triage` on the trace is a false positive.** It flags one block, but
  reverse-execution shows that block is dbghelp's per-thread **TLS** data,
  written legitimately during thread startup (constant from ~10% into the
  trace). No genuine adjacent-block overrun is visible in the trace.
- **Full PageHeap under TTD hangs** (guards every dbghelp alloc -> symbol
  enumeration crawls past the watchdog). **Standard PageHeap under TTD also
  hangs.** **PageHeap live doesn't reproduce** (changes timing away from the
  TTD window).
- **Four standalone DbgEng repros do not reproduce** (see `fh4-repro/`):
  `IDebugControl::Evaluate`; the `IDebugAdvanced2::Request(EXT_TDOP_SET_FROM_EXPR)`
  typed-data path `ExtRemoteTyped::Set` uses; that plus a nested
  `ExtCaptureOutput`-equivalent; and a minimal **extension DLL** loaded in cdb
  that recreates the `dbgeng -> extension -> dbgeng` reentrancy. All clean
  10-30 iters under TTD. So the trigger is **not** the bare engine symbol path;
  it needs the full `PyExtTest` process context.

### Source audit (the actual crash path, before any object reads)

The crash is in `PythonDumpFile` ctor -> `pysymfix` -> `ensureSymbolsLoaded`,
which runs before any object-type reads. Audited and **clean**:
`getSymbolPath` buffer sizing, `GetModuleNames` (`MAX_PATH`/`sizeof`),
`ExtCaptureOutput::Output` (overflow-guarded realloc/memcpy, LIFO nesting),
`makeAutoInterpreterState`, `RemoteType`, and the object-read buffers
(`readArray`, unicode, bytearray). No heap overrun found in PyExt code.

> Unrelated real bug spotted: `pysymfix.cpp getSymbolPath()` does
> `throw runtime_error("..." + hr)` — pointer arithmetic on a string literal
> (UB), on the `GetSymbolPath` failure path only. Not the crash cause; fix
> separately (use `std::to_string(hr)`).

### Current best assessment

Not the compiler. Not an obvious PyExt logic bug (audited). A timing-sensitive
heap corruption on the symbol-(re)load path that only manifests under TTD and
resists navigable capture. Most likely a latent corruption inside, or in
PyExt's interaction with, the `dbghelp`/`dbgeng` symbol engine (the
`engextcpp`/dbgeng layer Option-3 removes), exposed only by TTD's timing.
Artifacts kept under `fh4-repro/`: the clipped 700 MB TTD trace
(`ttd/crash/iter7_heap/PyExtTest01.run`) and the four standalone repros.

## Reproduce

```
# FH4 build (no /d2FH4-) is the crashing config:
msbuild PyExt.sln /p:Configuration=Release /p:Platform=x64 /p:DisableFH4=false
# Then run the object_types case repeatedly under TTD until it exits 0xC0000374:
ttd.exe -out <dir> -launch x64\Release\PyExtTest.exe [object_types] ^
        --object-types-dump-file test\scripts\object_types.dmp
```

(`Release|x64` is parameterized: `DisableFH4=false` removes `/d2FH4-`.)

## Action

Keep `/d2FH4-` only as a temporary unblock. Real fix belongs in the symbol-load
path (avoid / serialize expression evaluation during `Reload`), or report a
minimal repro to the `dbgeng`/`dbghelp` owners if the corruption proves to be
inside the engine. Remove the flag once fixed.
