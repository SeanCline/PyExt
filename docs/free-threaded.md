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
| `localsplus` / `f_executable` via stackref table | n/a | ⚠ best-effort (needs validation against a real dump) |
| Standalone `!pyinterpreterframe <addr>` on FT | ✅ | ⚠ `localsplus` resolves nothing — no owning thread context |
| Generator/coroutine embedded frame on FT | ✅ | ⚠ same; embedded frame has no owning-thread context |

## Validated versions

The free-threaded code paths are implemented to match CPython 3.13 / 3.14
internals (PEP 703 shipped in 3.13, became more prominent in 3.14). No
free-threaded dump has been exercised end-to-end yet — the GIL paths and
the build-mode discriminator are tested against a real Python 3.14 dump;
the free-threaded paths are code-inspected only and tested by the
`[free-threaded]`-tagged cases in `FreeThreadedTest.cpp`, which **SKIP**
when no FT dump is present.

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

- **Standalone frame inspection** — `!pyinterpreterframe <addr>` does not
  know which thread owns the frame, so it cannot resolve `tlbcIndex` (for
  bytecode start) or the stackref table (for `localsplus`). Bytecode
  falls back to `co_tlbc->entries[0]`, which is the main thread's copy
  and correct for the main thread only. `localsplus` decoded standalone
  comes back empty rather than wrong.
- **Generator / coroutine embedded frames** — same limitation. They carry
  no back-pointer to the owning thread, so the decoder treats them like
  the standalone case.
- **Stackref table field name** — the implementation probes
  `PyInterpreterState.open_stackrefs_table` then falls back to
  `stackrefs`. CPython has renamed this field across refactors; if neither
  name is exposed by the PDB the decoder returns `nullopt` for every slot
  and `localsplus` comes back empty.
- **Spike not run** — the plan flagged a one-day spike to verify the
  stackref table's `entries` array is reachable from a typical minidump
  (mimalloc may place it outside captured pages). That spike hasn't been
  run yet; if it fails, `localsplus` on FT degrades to empty for every
  frame and we document it as a Level B' rather than B.

## When something looks wrong

1. Confirm the banner is printing. If it isn't, the probe didn't fire —
   `_Py_NoneStruct` is unreachable in the dump or its symbol type doesn't
   expose `ob_tid`. Run `dt python3X!_object` in the debugger and check
   the field is present.
2. Refcounts negative or huge → the path probably didn't take the FT
   branch in `PyObject::refCount()`. Confirm `isFreeThreaded()` returns
   true at the prompt with `dx`.
3. `localsplus` empty → likely the stackref-table lookup failed. Try
   `dt python3X!PyInterpreterState` and look for either
   `open_stackrefs_table` or `stackrefs`; if the name on disk doesn't
   match the two strings the lookup probes, file a one-line note and
   we'll add the alternate to the fallback chain.
4. Wrong line numbers on a non-main thread → bytecode start probably
   fell back to `entries[0]` instead of the owning thread's slot. This
   path only kicks in when frames are walked from `PyThreadState`, so
   the issue is most likely `tlbc_index` not being read from the thread
   state. Check `dt python3X!_ts` for `tlbc_index`.
