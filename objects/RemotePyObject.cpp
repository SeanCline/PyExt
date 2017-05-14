#include "RemotePyObject.h"
#include "RemotePyTypeObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;


RemotePyObject::RemotePyObject(Offset objectAddress, const string& typeName /*= "PyObject"*/)
	: remoteObj_(make_shared<ExtRemoteTyped>(typeName.c_str(), objectAddress, true))
{
}


RemotePyObject::~RemotePyObject()
{
}


auto RemotePyObject::offset() -> Offset
{
	return remoteObj().GetPtr();
}


auto RemotePyObject::refCount() const -> SSize
{
	return remoteObj().Field("ob_refcnt").GetLong64();
}


auto RemotePyObject::type() const -> RemotePyTypeObject
{
	return RemotePyTypeObject(remoteObj().Field("ob_type").GetPtr());
}


auto RemotePyObject::typeName() const -> string
{
	return type().name();
}


auto RemotePyObject::repr(bool /*pretty*/) const -> string
{
	return "<" + typeName() + " object>";
}


auto RemotePyObject::remoteObj() const -> ExtRemoteTyped&
{
	return *remoteObj_;
}

