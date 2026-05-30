"""Runs the PyExtTest project against all installed python instances."""
import os, re, sys, subprocess, python_installations

if __name__ == '__main__':
    num_failed_tests = 0
    for installation in python_installations.get_python_installations():
         # Skip versions with no executable.
        if installation.exec_path == None:
            print("Skipping (no executable)", installation, end="\n\n", flush=True)
            continue
    
        # Only test against official CPython installations.
        if installation.company != "PythonCore":
            print("Skipping (PythonCore)", installation, end="\n\n", flush=True)
            continue
        
        # Also skip versions before 2.7 since they don't have symbols.
        # Strip non-digits (e.g. the "t" suffix on free-threaded tags like "3.14t")
        # so version parsing tolerates whatever the registry happens to hold.
        clean_sys_version = re.sub(r"[^\d.]", "", installation.sys_version)
        version = tuple(int(n) for n in clean_sys_version.split(".") if n)
        if version < (2, 7):
            print("Skipping (too old)", installation, end="\n\n", flush=True)
            continue

        # Create the dump files.
        print("Creating dump files with python executable:", installation.exec_path, flush=True)
        subprocess.check_call([installation.exec_path, "object_types.py"])
        subprocess.check_call([installation.exec_path, "fibonacci_test.py"])
        subprocess.check_call([installation.exec_path, "object_details.py"])
        subprocess.check_call([installation.exec_path, "localsplus_test.py"])
        subprocess.check_call([installation.exec_path, "pystack_all_test.py"])
        # Free-threaded-only: the script self-skips on GIL builds, so it's
        # safe to invoke here unconditionally.
        subprocess.check_call([installation.exec_path, "free_threaded_test.py"])
        
        # Run the tests against the dump files.
        print("Running tests with python executable:", installation.exec_path, flush=True)
        py_ext_test_exe = sys.argv[1] if len(sys.argv) > 1 else "../../x64/Debug/PyExtTest.exe"
        num_failed_tests += subprocess.call([py_ext_test_exe])

    sys.exit(num_failed_tests)
