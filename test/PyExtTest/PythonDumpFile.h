#pragma once

#include <string>
#include <vector>
#include <memory>
#include <wrl/client.h>

// Forward declarations.
struct IDebugClient;
struct IDebugControl;
struct IDebugSymbols2;

namespace PyExt::Remote {
	class PyFrame;
}

class PythonDumpFile {

public: // Construction/Destruction.
	PythonDumpFile(const std::string& dumpFilename);
	~PythonDumpFile();

public:
	auto getMainThreadFrames() const -> std::vector<std::shared_ptr<PyExt::Remote::PyFrame>>;

	// Python minor version of the dumped interpreter (e.g. 14 for 3.14), read
	// from the python module's file version. Returns 0 if it can't be read.
	// Lets version-sensitive tests gate assertions without new plumbing.
	auto pythonMinorVersion() const -> int;

public: // Debug interfaces.
	Microsoft::WRL::ComPtr<IDebugClient> pClient;
	Microsoft::WRL::ComPtr<IDebugControl> pControl;
	Microsoft::WRL::ComPtr<IDebugSymbols2> pSymbols;

private: // Helpers.
	auto createDebugInterfaces() -> void;
	auto setSymbolPath(const std::string& symbolPath) -> void;
	auto openDumpFile(const std::string & dumpFilename) -> void;
	auto loadPyExt() -> void;

};
