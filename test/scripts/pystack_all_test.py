"""Tests !pystack -all by capturing a dump while multiple threads are alive.

Three worker threads block on an Event while the main thread takes the dump,
so all three appear in the thread list with worker_function on their stacks.
"""

import threading
import win32debug

NUM_WORKERS = 3

_count_lock = threading.Lock()
_count = [0]
_all_ready = threading.Event()
_can_exit = threading.Event()


def worker_function():
    """Worker thread that is alive when the dump is taken."""
    with _count_lock:
        _count[0] += 1
        if _count[0] == NUM_WORKERS:
            _all_ready.set()
    _can_exit.wait()


if __name__ == '__main__':
    threads = [threading.Thread(target=worker_function) for _ in range(NUM_WORKERS)]
    for t in threads:
        t.daemon = True
        t.start()

    if not _all_ready.wait(timeout=10):
        raise RuntimeError("Timed out waiting for worker threads to start.")
    win32debug.dump_process("pystack_all_test.dmp")

    _can_exit.set()
    for t in threads:
        t.join()
