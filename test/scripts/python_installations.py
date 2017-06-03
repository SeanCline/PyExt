"""A small function for listing python instances installed on a Windows machine defined by PEP 514."""
import os, winreg, collections

# https://www.python.org/dev/peps/pep-0514/

def _enum_keys(key):
    i = 0
    while True:
        try:
            yield winreg.EnumKey(key, i)
            i += 1
        except OSError:
            break

def _get_value(key, value_name):
    try:
        return winreg.QueryValue(key, value_name)
    except FileNotFoundError:
        return None

"""A simple data-only object that represents a Python installation on disk."""
PythonInstallation = collections.namedtuple("PythonInstallation", "company name support_url version sys_version sys_arch exec_path")
        
def _create_python_installation(company, tag, tag_key):
    name = _get_value(tag_key, "DisplayName") or ("Python " + tag)
    url = _get_value(tag_key, "SupportUrl") or "http://www.python.org/"
    version = _get_value(tag_key, 'Version') or tag[:3]
    sys_version = _get_value(tag_key, 'SysVersion') or tag[:3]
    sys_arch= _get_value(tag_key, 'SysArchitecture') or None

    exec_path = None
    try:
        with winreg.OpenKey(tag_key, 'InstallPath') as ip_key:
            exec_path = (_get_value(ip_key, 'ExecutablePath')
                or os.path.join(_get_value(ip_key, None), 'python.exe'))
    except FileNotFoundError:
        pass
     
    return PythonInstallation(company, name, url, version, sys_version, sys_arch, exec_path)
        
def get_python_installations():
    """Returns a list of python executables on the machine."""
    python_installations = set()
    search_keys = [
        (winreg.HKEY_CURRENT_USER, r'Software\Python', 0),
        (winreg.HKEY_LOCAL_MACHINE, r'Software\Python', winreg.KEY_WOW64_64KEY),
        (winreg.HKEY_LOCAL_MACHINE, r'Software\Python', winreg.KEY_WOW64_32KEY),
    ]

    for hive, key, flags in search_keys:
        try:
            with winreg.OpenKeyEx(hive, key, access=winreg.KEY_READ | flags) as root_key:
                for company in _enum_keys(root_key):
                    with winreg.OpenKey(root_key, company) as company_key:
                        for tag in _enum_keys(company_key):
                            with winreg.OpenKey(company_key, tag) as tag_key:
                                python_installations.add(_create_python_installation(company, tag, tag_key))
        except FileNotFoundError:
            continue

    return python_installations

if __name__ == '__main__':
    print("\n".join(repr(x) for x in get_python_installations()))