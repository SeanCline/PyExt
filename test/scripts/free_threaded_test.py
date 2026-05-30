"""Tests PyExt's free-threaded (PEP 703 / Py_GIL_DISABLED) code paths.

This script generates `free_threaded_test.dmp` only when the running
interpreter is a free-threaded build. On a regular GIL build it exits
silently -- the C++ tests SKIP cleanly when the dump is absent. That keeps
this script safe to add to the universal `run_all_tests.py` loop without
producing per-version noise.

Kept deliberately ASCII-only: `run_all_tests.py` feeds every script to
every interpreter, including Python 2.7, which rejects non-ASCII source
bytes at parse time (before the GIL self-skip below can run).

The dump is captured from inside `capture()` so the bottom frame has a
non-trivial `localsplus`, which is what exercises Phase 6 (stackref
decoding). The named locals below are chosen so the C++ tests can make
specific assertions: an immortal singleton, a fresh mortal heap object,
and a dict to round-trip through `PyDictKeysObject`.
"""

import sys

# Self-skip on a GIL build. _is_gil_enabled() was added in Python 3.13.
is_gil_enabled = getattr(sys, "_is_gil_enabled", None)
if is_gil_enabled is None or is_gil_enabled():
    sys.exit(0)

import win32debug


def capture():
    none_local = None                 # Phase 2: refCount/isImmortal on a singleton.
    list_local = [1, 2, 3]            # Phase 2: small positive refcount, mortal.
    dict_local = {"a": 1, "b": 2}     # Phase 4: dict iteration via PyDictKeysObject.

    # Capture from inside the function so the bottom frame has a populated
    # localsplus for Phase 6 (stackref-decoded local reads).
    win32debug.dump_process("free_threaded_test.dmp")


if __name__ == "__main__":
    capture()
