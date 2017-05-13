#include "RemotePyVarObject.h"

#include <engextcpp.hpp>

#include <string>
using namespace std;

RemotePyVarObject::RemotePyVarObject(Offset objectAddress, const string& typeName /*= "PyVarObject"*/)
	: RemotePyObject(objectAddress, typeName)
{
}


auto RemotePyVarObject::size() const -> SSize
{
	return remoteObj().Field("ob_size").GetLong64();
}
