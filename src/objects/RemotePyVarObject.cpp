#include "RemotePyVarObject.h"

#include "utils/ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
using namespace std;

RemotePyVarObject::RemotePyVarObject(Offset objectAddress, const string& typeName /*= "PyVarObject"*/)
	: RemotePyObject(objectAddress, typeName)
{
}


auto RemotePyVarObject::size() const -> SSize
{
	auto sizeField = baseField("ob_size");
	return utils::readIntegral<SSize>(sizeField);
}
