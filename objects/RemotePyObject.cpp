#include "RemotePyObject.h"
#include "RemotePyTypeObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;


RemotePyObject::RemotePyObject(Offset objectAddress, const std::string& typeName /*= "PyObject"*/)
	: remoteObj_(make_shared<ExtRemoteTyped>(typeName.c_str(), objectAddress, true))
{
}


RemotePyObject::~RemotePyObject()
{
}


RemotePyObject::SSize RemotePyObject::refCount() const
{
	return remoteObj().Field("ob_refcnt").GetLong64();
}


RemotePyTypeObject RemotePyObject::type() const
{
	return RemotePyTypeObject(remoteObj().Field("ob_type").GetPtr());
}


string RemotePyObject::typeName() const
{
	return type().name();
}


string RemotePyObject::repr(bool /*pretty*/) const
{
	return "<" + typeName() + " object>";
}


ExtRemoteTyped& RemotePyObject::remoteObj() const
{
	return *remoteObj_;
}

