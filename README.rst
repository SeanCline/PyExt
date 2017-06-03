============================
WinDbg Extensions for Python
============================
.. image:: https://ci.appveyor.com/api/projects/status/f4osp2swvm1l25ct?svg=true
   :alt: Build Status
   :target: https://ci.appveyor.com/project/SeanCline/pyext
   
This debugger extension provides visualizations for Python objects and stacktraces when debugging the CPython interpreter. It helps with live debugging and post-mortem analysis of dump files.

The goal of this project is to provide a similar debugging experience in WinDbg/CDB/NTSD as `already exists in GDB <https://wiki.python.org/moin/DebuggingWithGdb>`_.

Currently, the extension is tested against 32bit and 64bit builds of Python versions 2.7, 3.3, 3.4, 3.5, and 3.6.

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

Use `~*e!pystack` to display the Python stack for all threads.
		
!pyobj
------
Displays the reference count, type, and value of a Python object, using similar formatting to Python's builtin `repr()` function.

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