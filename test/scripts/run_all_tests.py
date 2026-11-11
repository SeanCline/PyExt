"""Runs the PyExtTest project against all installed python instances."""
import os, sys, subprocess, python_installations

if __name__ == '__main__':
    num_failed_installations = 0
    for installation in python_installations.get_python_installations():
         # Skip versions with no executable.
        if installation.exec_path == None:
            print("Skipping (no executable)", installation, end="\n\n", flush=True)
            continue
    
        # Only test against official CPython installations.
        if installation.company != "PythonCore":
            print("Skipping (not PythonCore)", installation, end="\n\n", flush=True)
            continue
        
        # Also skip versions before 2.7 since they don't have symbols.
        version = tuple(int(n) for n in installation.sys_version.split("."))        
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
        
        # Run the tests against the dump files.
        print("Running tests with python executable:", installation.exec_path, flush=True)
        py_ext_test_exe = sys.argv[1] if len(sys.argv) > 1 else "../../x64/Debug/PyExtTest.exe"
        # The exit code only says whether there were failures, not how many.
        if subprocess.call([py_ext_test_exe]) != 0:
            num_failed_installations += 1

    sys.exit(1 if num_failed_installations else 0)
