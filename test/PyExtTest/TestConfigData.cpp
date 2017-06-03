#include "TestConfigData.h"

auto TestConfigData::instance() -> TestConfigData&
{
	static TestConfigData config;
	return config;
}
