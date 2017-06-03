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
	
	std::string symbolPath;
	auto symbolPathOrDefault() const -> std::string
	{
		return symbolPath.empty() ? "srv*c:\\symbols\\*http://pythonsymbols.sdcline.com/symbols/" : symbolPath;
	}
		
};
