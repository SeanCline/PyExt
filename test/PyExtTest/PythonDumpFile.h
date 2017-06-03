#pragma once

#include <string>
#include <vector>
#include <wrl/client.h>

// Forward declarations.
struct IDebugClient;
struct IDebugControl;
struct IDebugSymbols2;

namespace PyExt::Remote {
	class PyFrameObject;
}

class PythonDumpFile {

public: // Construction/Destruction.
	PythonDumpFile(const std::string& dumpFilename);
	~PythonDumpFile();

public:
	auto getMainThreadFrames() const -> std::vector<PyExt::Remote::PyFrameObject>;

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
