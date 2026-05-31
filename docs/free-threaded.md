# Free-threaded (PEP 703) build support

PyExt has partial support for debugging dumps captured from a free-threaded
(`Py_GIL_DISABLED`, `python3.Xt`) CPython interpreter. PEP 703 reshapes
enough of the C ABI that GIL-build assumptions silently produce wrong data
when applied to a free-threaded dump — refcounts, managed-dict pointers,
frame locals, and more. Rather than ship that, PyExt detects free-threaded
builds at command entry and switches its readers to the appropriate code
path where one exists, or emits a warning where one doesn't.

This document covers:

- How PyExt distinguishes the two builds.
- Which features work, which are best-effort, and which are unsupported.
- The end-to-end workflow for capturing and analysing a free-threaded dump.
- What to do when something looks wrong.

## Detection

At the top of every user-facing command (`!pyobj`, `!pystack`,
`!pyinterpreterframe`) PyExt calls `PyExt::isFreeThreaded()`, which probes
the loaded interpreter's symbols:

```cpp
ExtRemoteTyped("_Py_NoneStruct").HasField("ob_tid");
```

`ob_tid` (owning thread id) was added to `struct _object` only on the
free-threaded build, so its presence on the None singleton's static type is
a reliable discriminator. The probe runs on every call — no cache — so
swapping the loaded module with `!pysetautointerpreterstate` is reflected
immediately, with no stale "GIL" verdict masking the change.

When the probe says yes, every command prints a one-line note before its
output, listing the features that may be inaccurate.

## Feature matrix

| Feature | GIL build | Free-threaded |
|---|---|---|
| Build detection / banner | n/a | ✅ |
| `RefCount: N (immortal)` rendering | ✅ | ✅ |
| Managed dict, weakref slot offsets | ✅ | ✅ |
| Dict iteration (`PyDictKeysObject`) | ✅ | ✅ (audited; layout shift transparent) |
| Bytecode start via `co_tlbc[tlbcIndex]` | n/a | ✅ (per-thread; falls back to `entries[0]` if thread context is missing) |
| `localsplus` / `f_executable` decoding | ✅ tagged-pointer mask | ✅ same tagged-pointer mask (CPython 3.14 FT kept `_PyStackRef.bits`) |
| Standalone `!pyinterpreterframe <addr>` on FT | ✅ | ✅ same as GIL (tagged-pointer path needs no thread context) |
| Generator/coroutine embedded frame on FT | ✅ | ✅ same — works without thread context |

## Validated versions

The free-threaded code paths are validated end-to-end against a Python
3.14.5 free-threaded build (python.org installer, `python3.14t.exe`).
The `[free-threaded]`-tagged cases in `FreeThreadedTest.cpp` **SKIP**
when no FT dump is present, so contributors without a free-threaded
interpreter installed still get a green run.

**Note on PDB availability.** As of this writing, python.org does not
ship `python314t.pdb` on its public symbol server
(`pythonsymbols.sdcline.com`). The PDB lives next to the binary in the
install directory; point the test runner at it via
`--symbol-path "C:\path\to\Python314;srv*c:\symbols\*http://pythonsymbols.sdcline.com/symbols/"`
or set up your WinDbg symbol path the same way.

## Workflow

### 1. Install a free-threaded interpreter

The cleanest options on Windows:

- **python.org installer** (3.13+): Advanced Options → "Download
  free-threaded binaries". Installs `python3.Xt.exe` alongside `python.exe`
  in the same directory and registers it under PEP 514 as a separate tag
  (`HKLM\Software\Python\PythonCore\3.Xt`).
- **uv**: `uv python install 3.14t`. Doesn't touch the registry, but
  `test/scripts/python_installations.py` falls back to probing for a
  `python<X>.<Y>t.exe` companion next to any discovered install, so the
  test runner finds it anyway.
- **Build from source**: `PCbuild\build.bat -e --disable-gil`. The PDB
  ends up next to the binary; pass that directory to the test symbol path
  if you're debugging your own build.

### 2. Capture a dump

```cmd
> python3.14t.exe test\scripts\free_threaded_test.py
```

The script self-skips on a GIL build, so it's safe to invoke for every
Python install in `run_all_tests.py` without per-version branching. On a
free-threaded interpreter it produces `free_threaded_test.dmp`, captured
from inside a function with a small set of named locals (`none_local`,
`list_local`, `dict_local`) — the C++ tests assert against those.

### 3. Run the C++ test suite

```cmd
> PyExtTest.exe --free-threaded-dump-file path\to\free_threaded_test.dmp
```

The `[free-threaded]`-tagged cases in `FreeThreadedTest.cpp` exercise:

- Build detection (`isFreeThreaded() == true`).
- Pre-header layout (`headerSize()` larger than the GIL build's; managed
  dict slot at `-1 * pointerSize`).
- Refcount split + immortality on the None singleton.
- Stackref-decoded localsplus (the Phase 6 gate — passes if any named
  local resolves to a non-null PyObject).
- Dict iteration through the FT-layout `PyDictKeysObject`.

When the dump file is missing the suite SKIPs each case with a helpful
message rather than failing, so CI stays green on machines without a
free-threaded interpreter installed.

### 4. Inspect interactively

```
0:000> .load pyext
0:000> !pystack
Note: free-threaded (Py_GIL_DISABLED) build detected. PyExt's support for
free-threaded interpreters is in progress; the following may be inaccurate
or missing in this output: refcounts, managed dicts, frame localsplus,
generator/coroutine state.
Thread 0:
    File "...\free_threaded_test.py", line 26, in capture
    File "...\free_threaded_test.py", line 30, in <module>
```

## Known limitations

- **Standalone frame inspection on non-main threads** —
  `!pyinterpreterframe <addr>` does not know which thread owns the
  frame, so on the free-threaded build it cannot resolve `tlbcIndex`
  for the per-thread bytecode copy. It falls back to
  `co_tlbc->entries[0]`, which is the main thread's copy. For frames
  belonging to the main thread this is exactly right; for other threads
  the byte offsets within the bytecode may not match, producing
  approximate line numbers when specializations differ across threads.
  Walking the same frame from `!pystack` (which goes through
  `PyThreadState`) does not have this limitation.
- **Generator / coroutine embedded frames on non-main threads** — same
  caveat. The embedded `_PyInterpreterFrame` carries no back-pointer to
  the owning thread, so we fall back to `entries[0]` for bytecode.

### Subsumed by reality

The original plan called out two risks that turned out not to apply on
the shipped 3.14 free-threaded build:

- *Stackref table reachability* — PEP 703's early sketch used a
  per-interpreter table indexed by `_PyStackRef.index`. CPython 3.14
  shipped with `_PyStackRef` as a plain tagged-pointer union (a single
  `bits` member), so there is no table to walk and the `localsplus`
  decode is just the same tagged-pointer mask as the GIL build. A
  fallback path through `PyInterpreterState::stackrefEntry` remains in
  the code in case a future CPython resurrects the index design; today
  it never runs.
- *Spike on entries reachability* — moot for the same reason. The
  Level B' fallback ("localsplus unavailable on free-threaded") is not
  needed in 3.14.

## When something looks wrong

1. Confirm the banner is printing. If it isn't, the probe didn't fire —
   `_Py_NoneStruct` is unreachable in the dump or its symbol type doesn't
   expose `ob_tid`. Run `dt python3X!_object` in the debugger and check
   the field is present.
2. Refcounts negative or huge → the path probably didn't take the FT
   branch in `PyObject::refCount()`. Confirm `isFreeThreaded()` returns
   true at the prompt with `dx`.
3. `localsplus` empty → if CPython has reintroduced the `_PyStackRef.index`
   design, `localsplus()` will route through `stackrefEntry`. Check
   `dt python3X!_PyStackRef` for an `index` field, and if present check
   `dt python3X!PyInterpreterState` for the table — the implementation
   probes `open_stackrefs_table` then `stackrefs`. If the actual field
   name differs, file a one-line note and we'll add the alternate to the
   fallback chain.
4. Wrong line numbers on a non-main thread → bytecode start probably
   fell back to `entries[0]` instead of the owning thread's slot. This
   path only kicks in when frames are walked from `PyThreadState`, so
   the issue is most likely `tlbc_index` not being read from the thread
   state. Check `dt python3X!_ts` for `tlbc_index`.
