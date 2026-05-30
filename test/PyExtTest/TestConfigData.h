#pragma once

#include <string>

struct TestConfigData {

	static auto instance()->TestConfigData&;

	std::string objectTypesDumpFileName;
	auto objectTypesDumpFileNameOrDefault() const -> std::string
	{
		return objectTypesDumpFileName.empty() ? "object_types.dmp" : objectTypesDumpFileName;
	}

	std::string fibonacciDumpFileName;
	auto fibonaciiDumpFileNameOrDefault() const -> std::string
	{
		return fibonacciDumpFileName.empty() ? "fibonacci_test.dmp" : fibonacciDumpFileName;
	}
	
	std::string objectDetailsDumpFileName;
	auto objectDetailsDumpFileNameOrDefault() const -> std::string
	{
		return objectDetailsDumpFileName.empty() ? "object_details.dmp" : objectDetailsDumpFileName;
	}

	std::string localsplusDumpFileName;
	auto localsplusDumpFileNameOrDefault() const -> std::string
	{
		return localsplusDumpFileName.empty() ? "localsplus_test.dmp" : localsplusDumpFileName;
	}

	std::string pystackAllDumpFileName;
	auto pystackAllDumpFileNameOrDefault() const -> std::string
	{
		return pystackAllDumpFileName.empty() ? "pystack_all_test.dmp" : pystackAllDumpFileName;
	}

	std::string freeThreadedDumpFileName;
	auto freeThreadedDumpFileNameOrDefault() const -> std::string
	{
		return freeThreadedDumpFileName.empty() ? "free_threaded_test.dmp" : freeThreadedDumpFileName;
	}

	std::string symbolPath;
	auto symbolPathOrDefault() const -> std::string
	{
		return symbolPath.empty() ? "srv*c:\\symbols\\*http://pythonsymbols.sdcline.com/symbols/" : symbolPath;
	}

};
