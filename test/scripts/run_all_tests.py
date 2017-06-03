"""Runs the PyExtTest project against all installed python instances."""
import sys, subprocess, python_installations

if __name__ == '__main__':
    num_failed_tests = 0
    for installation in python_installations.get_python_installations():
        # Only test against official CPython installations.
        # Also skip versions before 2.7 since they don't have symbols.
        version = tuple(int(n) for n in installation.version.split("."))        
        if installation.company != "PythonCore" or version < (2, 7):
            print("Skipping", installation)
            print()
            continue
        
        # Create the dump files.
        print("Creating dump files with python executable:", installation.exec_path)
        subprocess.check_call([installation.exec_path, "object_types.py"])
        subprocess.check_call([installation.exec_path, "fibonacci_test.py"])
        
        # Run the tests against the dump files.
        py_ext_test_exe = sys.argv[1] if len(sys.argv) > 1 else "../../x64/Debug/PyExtTest.exe"
        num_failed_tests += subprocess.call(py_ext_test_exe)

sys.exit(num_failed_tests)