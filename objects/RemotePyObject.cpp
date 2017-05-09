#include "RemotePyObject.h"

#include "util/to_string.h"
#include <engextcpp.hpp>
#include <string>
using namespace std;


RemotePyObject::RemotePyObject(Offset objectAddress, const std::string& typeName /*= "PyObject"*/)
	: remoteObj_(typeName.c_str(), objectAddress, true)
{
}


RemotePyObject::~RemotePyObject()
{
}


long long RemotePyObject::getRefCount() const
{
	return remoteObj_.Field("ob_refcnt").GetLong64();
}


string RemotePyObject::getTypeName() const
{
	ExtBuffer<char> buff;
	auto remoteTypeObj = remoteObj_.Field("ob_type");
	remoteTypeObj.Field("tp_name").Dereference().GetString(&buff);
	return to_string(buff);
}


string RemotePyObject::repr(bool pretty) const
{
	return "<Unknown Object Type: " + getTypeName() + ">";
}


ExtRemoteTyped RemotePyObject::remoteObj() const
{
	return remoteObj_;
}

