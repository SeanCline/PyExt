import win32debug

# If we are being run directly, test out some debug functionality.
if __name__ == "__main__":
	if win32debug.is_debugger_present():
		win32debug.output_debug_string("A debugger is attached!\n")
	else:
		win32debug.output_debug_string("No debugger attached.\n")
		
	win32debug.output_debug_string("About to trigger break.\n")
	win32debug.debug_break()
