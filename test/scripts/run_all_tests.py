"""Runs the PyExtTest project against all installed python instances."""
import sys, subprocess, python_installations

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
        version = tuple(int(n) for n in installation.sys_version.split("."))        
        if version < (2, 7):
            print("Skipping (too old)", installation, end="\n\n", flush=True)
            continue

        # Create the dump files.
        print("Creating dump files with python executable:", installation.exec_path, flush=True)
        subprocess.check_call([installation.exec_path, "object_types.py"])
        subprocess.check_call([installation.exec_path, "fibonacci_test.py"])
        
        # Run the tests against the dump files.
        py_ext_test_exe = sys.argv[1] if len(sys.argv) > 1 else "../../x64/Debug/PyExtTest.exe"
        num_failed_tests += subprocess.call(py_ext_test_exe)

sys.exit(num_failed_tests)