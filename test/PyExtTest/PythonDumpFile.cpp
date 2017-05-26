#include "PythonDumpFile.h"

#include <PyInterpreterState.h>
#include <PyThreadState.h>
#include <PyFrameObject.h>
using namespace PyExt::Remote;

#include <DbgEng.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <cstdlib>
#include <stdexcept>
#include <string>
using namespace std;


PythonDumpFile::PythonDumpFile(const std::string& dumpFilename)
{
	createDebugInterfaces();
	openDumpFile(dumpFilename);
	setSymbolPath(getPythonPath() + ";srv*http://msdl.microsoft.com/download/symbols");
	loadPyExt();
}


PythonDumpFile::~PythonDumpFile()
{
}

auto PythonDumpFile::getMainThreadFrames() const -> std::vector<PyExt::Remote::PyFrameObject>
{
	auto interpState = PyInterpreterState::makeAutoInterpreterState();
	auto threads = interpState->allThreadStates();
	if (threads.empty())
		throw std::runtime_error("No threads in process.");

	return threads.back().allFrames();
}


auto PythonDumpFile::createDebugInterfaces() -> void
{
	HRESULT hr = DebugCreate(__uuidof(IDebugClient), reinterpret_cast<void**>(pClient.GetAddressOf()));
	if (FAILED(hr))
		throw runtime_error("Failed to create IDebugClient.");

	if (FAILED(pClient.As(&pControl)))
		throw runtime_error("Failed to query IDebugControl.");

	if (FAILED(pClient.As(&pSymbols)))
		throw runtime_error("Failed to query IDebugSymbols2.");
}


auto PythonDumpFile::setSymbolPath(const string& symbolPath) -> void
{
	HRESULT hr = pSymbols->SetSymbolPath(symbolPath.c_str());
	if (FAILED(hr))
		throw runtime_error("Failed to SetSymbolPath.");

	pSymbols->Reload("");
	pSymbols->Reload("/f python*");
}


auto PythonDumpFile::openDumpFile(const std::string & dumpFilename) -> void
{
	HRESULT hr = pClient->OpenDumpFile(dumpFilename.c_str());
	if (FAILED(hr))
		throw runtime_error("Failed to OpenDumpFile.");

	// The debug session isn't completely open until we call WaitForEvent.
	hr = pControl->WaitForEvent(DEBUG_WAIT_DEFAULT, 3000 /*ms*/);
	if (FAILED(hr) || hr == S_FALSE)
		throw runtime_error("Failed to WaitForEvent.");
}


auto PythonDumpFile::loadPyExt() -> void
{
	// Manually call ::LoadLibrary to use the typical dll search order since
	// IDebugControl::AddExtension doesn't look everywhere for the extension.
	auto pyExtModule = ::LoadLibrary("pyext");
	if (pyExtModule == 0)
		throw runtime_error("Could not load pyext.dll");

	// Once the extension dll is loaded, tell DbgEng to initialize it.
	constexpr auto bufferSize = 1024;
	string pyExtFile(bufferSize, '\0');
	auto nSize = ::GetModuleFileName(pyExtModule, pyExtFile.data(), bufferSize);
	if (nSize == bufferSize)
		throw runtime_error("GetModuleFileName buffer too small.");

	pyExtFile.resize(nSize);

	ULONG64 pyExtHandle = 0;
	HRESULT hr = pControl->AddExtension(pyExtFile.data(), 0, &pyExtHandle);
	if (FAILED(hr))
		throw runtime_error("Failed to load PyExt extension.");
}


auto PythonDumpFile::getPythonPath() const -> std::string
{
	ULONG moduleIndex = 0;
	ULONG64 moduleBase = 0;
	HRESULT hr = pSymbols->GetModuleByModuleName("python", 0, &moduleIndex, &moduleBase);
	if (FAILED(hr))
		throw runtime_error("GetModuleByModuleName failed find python module.");

	ULONG moduleNameSize = 2048;
	string moduleName(moduleNameSize, '\0');
	hr = pSymbols->GetModuleNameString(DEBUG_MODNAME_IMAGE, moduleIndex, moduleBase, moduleName.data(), moduleNameSize, &moduleNameSize);
	if (FAILED(hr))
		throw runtime_error("GetModuleNameString failed for python module.");
	moduleName.resize(moduleNameSize);

	string drive(_MAX_DRIVE, '\0');
	string dir(_MAX_DIR, '\0');
	_splitpath_s(moduleName.c_str(), drive.data(), drive.size(), dir.data(), dir.size(), nullptr, 0, nullptr, 0);
	drive.resize(drive.find('\0', 0));
	dir.resize(dir.find('\0', 0));
	return drive + dir;
}
