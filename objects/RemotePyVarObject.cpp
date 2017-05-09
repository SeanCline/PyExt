#include "RemotePyVarObject.h"

#include <engextcpp.hpp>


RemotePyVarObject::RemotePyVarObject(Offset objectAddress, const std::string& typeName /*= "PyVarObject"*/)
	: RemotePyObject(objectAddress, typeName)
{
}


RemotePyVarObject::SSize RemotePyVarObject::getSize() const
{
	return remoteObj().Field("ob_size").GetLong64();
}
