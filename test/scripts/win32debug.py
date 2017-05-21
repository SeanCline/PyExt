import sys, ctypes, msvcrt
from multiprocessing.pool import ThreadPool


class MiniDumpType:
	"""MiniDump flags as defined on `MSDN <https://msdn.microsoft.com/en-us/library/windows/desktop/ms680519(v=vs.85).aspx>`_"""
	
	Normal                          = 0x00000000
	WithDataSegs                    = 0x00000001
	WithFullMemory                  = 0x00000002
	WithHandleData                  = 0x00000004
	FilterMemory                    = 0x00000008
	ScanMemory                      = 0x00000010
	WithUnloadedModules             = 0x00000020
	WithIndirectlyReferencedMemory  = 0x00000040
	FilterModulePaths               = 0x00000080
	WithProcessThreadData           = 0x00000100
	WithPrivateReadWriteMemory      = 0x00000200
	WithoutOptionalData             = 0x00000400
	WithFullMemoryInfo              = 0x00000800
	WithThreadInfo                  = 0x00001000
	WithCodeSegs                    = 0x00002000
	WithoutAuxiliaryState           = 0x00004000
	WithFullAuxiliaryState          = 0x00008000
	WithPrivateWriteCopyMemory      = 0x00010000
	IgnoreInaccessibleMemory        = 0x00020000
	WithTokenInformation            = 0x00040000
	WithModuleHeaders               = 0x00080000
	FilterTriage                    = 0x00100000
	ValidTypeFlags                  = 0x001fffff


class StandardAccessRights:
	"""Defines the access rights common to most types of Win32 handle."""
	"""`<https://msdn.microsoft.com/en-us/library/windows/desktop/aa379607(v=vs.85).aspx>`_"""
	
	DELETE = 0x00010000
	"""Required to delete the object."""
	
	READ_CONTROL = 0x00020000
	"""Required to read information in the security descriptor for the object, not including the information in the SACL."""
	"""To read or write the SACL, you must request the ACCESS_SYSTEM_SECURITY access right. For more information, see SACL Access Right."""
	
	SYNCHRONIZE = 0x00100000
	"""The right to use the object for synchronization. This enables a thread to wait until the object is in the signaled state."""
	
	WRITE_DAC = 0x00040000
	"""Required to modify the DACL in the security descriptor for the object."""
	
	WRITE_OWNER = 0x00080000
	"""Required to change the owner in the security descriptor for the object."""

	STANDARD_RIGHTS_ALL = DELETE | READ_CONTROL | WRITE_DAC | WRITE_OWNER | SYNCHRONIZE
	"""Combines DELETE, READ_CONTROL, WRITE_DAC, WRITE_OWNER, and SYNCHRONIZE access."""
	
	STANDARD_RIGHTS_REQUIRED = DELETE | READ_CONTROL | WRITE_DAC | WRITE_OWNER
	"""Combines DELETE, READ_CONTROL, WRITE_DAC, and WRITE_OWNER access."""
	
	STANDARD_RIGHTS_EXECUTE	= READ_CONTROL
	STANDARD_RIGHTS_READ = READ_CONTROL
	STANDARD_RIGHTS_WRITE = READ_CONTROL


class ProcessAccessRights:
	"""Defines the access rights specific to Win32 process handles."""
	"""`<https://msdn.microsoft.com/en-us/library/windows/desktop/ms684880(v=vs.85).aspx>`_"""
	
	PROCESS_CREATE_PROCESS = 0x0080
	"""Required to create a process."""
	
	PROCESS_CREATE_THREAD = 0x0002
	"""Required to create a thread."""
	
	PROCESS_DUP_HANDLE = 0x0040
	"""Required to duplicate a handle using DuplicateHandle."""
	
	PROCESS_QUERY_INFORMATION = 0x0400
	"""Required to retrieve certain information about a process, such as its token, exit code, and priority class = see OpenProcessToken)."""
	
	PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
	"""Required to retrieve certain information about a process. (GetExitCodeProcess, GetPriorityClass, IsProcessInJob, QueryFullProcessImageName)."""
	
	PROCESS_SET_INFORMATION = 0x0200
	"""Required to set certain information about a process, such as its priority class = see SetPriorityClass)."""
	
	PROCESS_SET_QUOTA = 0x0100
	"""Required to set memory limits using SetProcessWorkingSetSize."""
	
	PROCESS_SUSPEND_RESUME = 0x0800
	"""Required to suspend or resume a process."""
	
	PROCESS_TERMINATE = 0x0001
	"""Required to terminate a process using TerminateProcess."""
	
	PROCESS_VM_OPERATION = 0x0008
	"""Required to perform an operation on the address space of a process = see VirtualProtectEx and WriteProcessMemory)."""
	
	PROCESS_VM_READ = 0x0010
	"""Required to read memory in a process using ReadProcessMemory."""
	
	PROCESS_VM_WRITE = 0x0020
	"""Required to write to memory in a process using WriteProcessMemory."""
	
	SYNCHRONIZE = 0x00100000
	"""Required to wait for the process to terminate using the wait functions."""
	
	PROCESS_ALL_ACCESS = StandardAccessRights.STANDARD_RIGHTS_REQUIRED | StandardAccessRights.SYNCHRONIZE | 0xFFF
	"""All possible access rights for a process object."""


class Win32Error(Exception):
	"""Signifies an error when calling a Win32 function. Includes the function name and value of GetLastError()."""
	def __init__(self, function_name, last_error=None):
		if last_error == None:
			last_error = ctypes.GetLastError()
		
		self.function_name, self.last_error = function_name, last_error
		super(Win32Error, self).__init__(str(function_name) + " failed with GetLastError=" + str(last_error))


def _mini_dump_write_dump(process_id, dump_file_handle, dump_type):
	"""Wraps dbghelp.dll's MiniDumpWriteDump."""
	
	# TODO: Determine the rights needed based on dump_type.
	process_rights = ProcessAccessRights.PROCESS_QUERY_INFORMATION | ProcessAccessRights.PROCESS_VM_READ
	process_handle = ctypes.windll.kernel32.OpenProcess(process_rights, False, process_id)
	if process_handle == 0:
		raise Win32Error("OpenProcess")

	succeeded = ctypes.windll.dbghelp.MiniDumpWriteDump(process_handle, process_id, dump_file_handle, dump_type, None, None, None)
	if not succeeded:
		raise Win32Error("MiniDumpWriteDump")


def _to_w_string(string_object):
	"""Returns the object unchanged if it is already unicode (wide-string), otherwise, converts to unicode."""
	return string_object if (sys.version_info.major != 2) else unicode(string_object)


def dump_process(dump_file_name, process_id=None):
	"""Writes a dump of a given pid (or the current process if not specified) to a file."""
	
	current_process_id = ctypes.windll.kernel32.GetCurrentProcessId()
	if process_id == None:
		process_id = current_process_id
	
	dump_type = MiniDumpType.WithFullMemory | MiniDumpType.WithFullMemoryInfo | MiniDumpType.WithThreadInfo
	
	with open(dump_file_name, "w+") as dump_file:
		dump_file_handle = msvcrt.get_osfhandle(dump_file.fileno())

		# Do the dump from a separate thread so the current thread's stack is captured properly.
		pool = ThreadPool(processes=1)
		result = pool.apply_async(_mini_dump_write_dump, (process_id, dump_file_handle, dump_type))
		result.get() # Wait and raise any errors that occured during the dump.


def is_debugger_present():
	"""Returns true when the current process has a debugger attached."""
	return ctypes.windll.kernel32.IsDebuggerPresent() != 0


def output_debug_string(debug_string):
	"""Prints a string in any attached debuggers or tools that intercept debug strings, such as DbgView.exe"""
	ctypes.windll.kernel32.OutputDebugStringW(_to_w_string(debug_string))


def debug_break():
	"""Triggers a breakpoint in an attached debugger, or halts the process if a debugger is not attached."""
	ctypes.windll.kernel32.DebugBreak()


# If we are being run directly, write a dump of the current process.
if __name__ == "__main__":
    dump_process(__file__ + ".dmp")
