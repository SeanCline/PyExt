"""This test exercises PyExt's stack frame logic.

It is intentionally a bad implementation since the test suite uses it to make
assertions about the number of frames on the stack, and the line numbers of
each frame.

Note: When modifying this file, changes to line numbers may require the tests
to be updated.
"""

import win32debug

def recursive_fib(n):
    """Returns the nth fibonacci number, calulated recursively."""
    
    # Write a highest point of the call stack.
    if n == 1:
        win32debug.dump_process("fibonacci_test.dmp") # Asserted line 18.
        exit(0)
    
    if n < 2:
        return n
    else:
        return recursive_fib(n-1) + recursive_fib(n-2) # Asserted line 24.
        

if __name__ == '__main__':
    print(recursive_fib(500)) # Asserted line 28.