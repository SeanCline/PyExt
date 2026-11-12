============================
WinDbg Extensions for Python
============================
.. image:: https://github.com/SeanCline/PyExt/actions/workflows/ci.yml/badge.svg?branch=master
   :alt: Build Status
   :target: https://github.com/SeanCline/PyExt/actions/workflows/ci.yml
   
This debugger extension provides visualizations for Python objects and stacktraces when debugging the CPython interpreter. It helps with live debugging and post-mortem analysis of dump files.

The goal of this project is to provide a similar debugging experience in WinDbg/CDB/NTSD as `already exists in GDB <https://wiki.python.org/moin/DebuggingWithGdb>`_.

Currently, the extension is tested against 32bit and 64bit builds of Python versions 2.7, and 3.3 through 3.15.

Installation
============
 - Build from source or download binaries from the Releases page
 - Copy `pyext.dll` of the appropriate bitness into `<WINDBG_ROOT>\<BITNESS>\winext`
 - Ensure you have the latest Microsoft Visual C++ Redistributable installed

Extension Commands
==================

!pystack
--------
Displays the Python callstack for the current thread.

Example usage:
^^^^^^^^^^^^^^
.. code-block::

    0:000> !pystack
    Thread 0:
        File "C:\Python36\lib\threading.py", line 1072, in _wait_for_tstate_lock
        File "C:\Python36\lib\threading.py", line 1056, in join
        File "scripts\win32debug.py", line 148, in _launch_and_wait
        File "scripts\win32debug.py", line 175, in dump_process
        File ".\fibonacci_test.py", line 18, in recursive_fib
        File ".\fibonacci_test.py", line 18, in recursive_fib
        File ".\fibonacci_test.py", line 18, in recursive_fib
        File ".\fibonacci_test.py", line 28, in <module>

Use `!pystack -all` or `~*e!pystack` to display the Python stack for all threads.

`!pystack <address>` displays the stack starting from a frame object, or from an interpreter frame on Python 3.11 and later.

In WinDbg, each line is followed by links that inspect that frame's locals and globals.

!pyobj
------
Displays the reference count, type, and value of a Python object, using similar formatting to Python's builtin `repr()` function.

The argument is an address, or any expression the debugger can evaluate to one.
The expressions in these examples only apply to Python 3.6 and earlier.
On later versions the links printed by `!pystack` are the easiest way to get to an object.

Example usage:
^^^^^^^^^^^^^^
.. code-block::

    0:000> !pyobj autoInterpreterState->tstate_head->frame->f_code
    PyCodeObject at address: 000001fc`b6a87f60
    RefCount: 2
    Type: code
    Repr: <code object, file "scripts\win32debug.py", line 120>

.. code-block::

    0:000> !pyobj autoInterpreterState->tstate_head->frame->f_globals
    PyDictObject at address: 000001fc`b6ba6bd0
    RefCount: 15
    Type: dict
    Repr: {
        '__name__': 'win32debug',
        '__doc__': 'Wrappers around various Win32 APIs debugging.',
    # ...
    }

!pyinterpreterframe
-------------------
Displays the local variables of an interpreter frame, which is where Python 3.11 and later keep the state of a running function.
`!pystack` prints a link to this command for each frame.

Example usage:
^^^^^^^^^^^^^^
.. code-block::

    0:000> !pyinterpreterframe 0x000001733487b230
        Details:
    localsplus: {
        'dump_file_name': 'fibonacci_test.dmp',
        'process_id': 7324,
        'dump_type': 6146,
    }

!pysymfix
---------
Adds the Python symbol server to the symbol path, along with the directory of any Python DLL that has its PDB next to it, then reloads symbols.
Run this first if the other commands report that Python symbols could not be loaded.

!pysetautointerpreterstate
--------------------------
Manually provides an expression for the `PyInterpreterState` to use, for when PyExt fails to find it automatically.
Run it without an argument to go back to finding it automatically.
