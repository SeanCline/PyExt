#include "RemotePyObject.h"

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


RemotePyObject::SSize RemotePyObject::refCount() const
{
	return remoteObj().Field("ob_refcnt").GetLong64();
}


string RemotePyObject::typeName() const
{
	// TODO: Construct a type object here instead of digging into the type's name member.
	ExtBuffer<char> buff;
	remoteObj().Field("ob_type").Field("tp_name").Dereference().GetString(&buff);
	return buff.GetBuffer();
}


string RemotePyObject::repr(bool pretty) const
{
	return "<" + typeName() + " object>";
}


ExtRemoteTyped RemotePyObject::remoteObj() const
{
	return remoteObj_;
}

