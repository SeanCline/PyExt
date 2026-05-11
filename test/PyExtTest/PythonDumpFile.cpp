#include "PythonDumpFile.h"
#include "TestConfigData.h"

#include <PyInterpreterState.h>
#include <PyThreadState.h>
#include <PyFrameObject.h>
using namespace PyExt::Remote;

#include <DbgEng.h>
#include <DbgHelp.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

#include <cstdlib>
#include <format>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
using namespace std;

namespace {
	string hresult_to_string(HRESULT hr)
	{
		return to_string(hr);
	}
}

PythonDumpFile::PythonDumpFile(const std::string& dumpFilename)
{
	createDebugInterfaces();
	setSymbolPath(TestConfigData::instance().symbolPathOrDefault());
	openDumpFile(dumpFilename);
	pSymbols->Reload("/f python*");
	loadPyExt();

	// Print symbol info once per test run to help diagnose symbol loading failures.
	static bool symbolInfoPrinted = false;
	if (!symbolInfoPrinted) {
		symbolInfoPrinted = true;

		cout << "Symbol path: " << TestConfigData::instance().symbolPathOrDefault() << "\n";

		auto getPdbGuid = [&](ULONG moduleIndex) -> string {
			DEBUG_MODULE_PARAMETERS params = {};
			if (FAILED(pSymbols->GetModuleParameters(1, nullptr, moduleIndex, &params)))
				return "(unavailable)";
			ComPtr<IDebugAdvanced3> pAdvanced;
			if (FAILED(pClient.As(&pAdvanced)))
				return "(unavailable)";
			IMAGEHLP_MODULEW64 info = {};
			info.SizeOfStruct = sizeof(info);
			if (FAILED(pAdvanced->GetSymbolInformation(DEBUG_SYMINFO_IMAGEHLP_MODULEW64,
			                                           params.Base, 0,
			                                           &info, sizeof(info), nullptr,
			                                           nullptr, 0, nullptr)))
				return "(unavailable)";
			const GUID& g = info.PdbSig70;
			return std::format("{:08X}{:04X}{:04X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{}",
			                   g.Data1, g.Data2, g.Data3,
			                   g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
			                   g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7],
			                   info.PdbAge);
		};

		ULONG moduleCount = 0, unloadedCount = 0;
		if (SUCCEEDED(pSymbols->GetNumberModules(&moduleCount, &unloadedCount))) {
			for (ULONG i = 0; i < moduleCount; i++) {
				char modName[MAX_PATH] = {};
				if (FAILED(pSymbols->GetModuleNameString(DEBUG_MODNAME_MODULE, i, 0, modName, sizeof(modName), nullptr)))
					continue;
				if (_strnicmp(modName, "python", 6) != 0)
					continue;

				char imagePath[MAX_PATH] = {};
				pSymbols->GetModuleNameString(DEBUG_MODNAME_IMAGE, i, 0, imagePath, sizeof(imagePath), nullptr);

				char symFile[MAX_PATH] = {};
				pSymbols->GetModuleNameString(DEBUG_MODNAME_SYMBOL_FILE, i, 0, symFile, sizeof(symFile), nullptr);

				string fileVersion = "(unknown)";
				ComPtr<IDebugSymbols3> pSymbols3;
				if (SUCCEEDED(pClient.As(&pSymbols3))) {
					VS_FIXEDFILEINFO vfi = {};
					if (SUCCEEDED(pSymbols3->GetModuleVersionInformation(i, 0, "\\", &vfi, sizeof(vfi), nullptr))) {
						fileVersion = to_string(HIWORD(vfi.dwFileVersionMS)) + "." +
						              to_string(LOWORD(vfi.dwFileVersionMS)) + "." +
						              to_string(HIWORD(vfi.dwFileVersionLS)) + "." +
						              to_string(LOWORD(vfi.dwFileVersionLS));
					}
				}

				cout << "Python module: " << modName << " (image: " << imagePath << ")\n"
				     << "  file version: " << fileVersion << "\n"
				     << "  pdb guid:     " << getPdbGuid(i) << "\n"
				     << "  symbol file:  " << (symFile[0] ? symFile : "(none)") << "\n";
			}
		}
		cout << flush;
	}
}


PythonDumpFile::~PythonDumpFile()
{
}


auto PythonDumpFile::getMainThreadFrames() const -> std::vector<std::shared_ptr<PyExt::Remote::PyFrame>>
{
	auto interpState = PyInterpreterState::makeAutoInterpreterState();

	// CPython prepends new threads to the head of the tstate linked list, so
	// the main thread (created first) is always the last one we iterate to.
	std::optional<PyThreadState> lastThread;
	for (auto&& tstate : interpState->allThreadStates()) {
		lastThread = tstate;
	}

	if (!lastThread.has_value())
		throw std::runtime_error("No threads in process.");

	return lastThread->allFrames();
}


auto PythonDumpFile::createDebugInterfaces() -> void
{
	HRESULT hr = DebugCreate(__uuidof(IDebugClient), reinterpret_cast<void**>(pClient.GetAddressOf()));
	if (FAILED(hr))
		throw runtime_error("Failed to create IDebugClient. hr=" + hresult_to_string(hr));

	if (FAILED(pClient.As(&pControl)))
		throw runtime_error("Failed to query IDebugControl. hr=" + hresult_to_string(hr));

	if (FAILED(pClient.As(&pSymbols)))
		throw runtime_error("Failed to query IDebugSymbols2. hr=" + hresult_to_string(hr));
}


auto PythonDumpFile::setSymbolPath(const string& symbolPath) -> void
{
	HRESULT hr = pSymbols->SetSymbolPath(symbolPath.c_str());
	if (FAILED(hr))
		throw runtime_error("Failed to SetSymbolPath. hr=" + hresult_to_string(hr));
}


auto PythonDumpFile::openDumpFile(const std::string & dumpFilename) -> void
{
	HRESULT hr = pClient->OpenDumpFile(dumpFilename.c_str());
	if (FAILED(hr))
		throw runtime_error("OpenDumpFile failed with dumpFilename=" + dumpFilename);

	// The debug session isn't completely open until we call WaitForEvent.
	hr = pControl->WaitForEvent(DEBUG_WAIT_DEFAULT, 3000 /*ms*/);
	if (FAILED(hr) || hr == S_FALSE)
		throw runtime_error("Failed to WaitForEvent. hr=" + hresult_to_string(hr));
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
		throw runtime_error("Failed to load PyExt extension. hr=" + hresult_to_string(hr));
}
