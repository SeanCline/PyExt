#include "RemotePyObject.h"

#include "RemotePyTypeObject.h"
#include "utils/ExtHelpers.h"

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
	auto refcnt = baseField("ob_refcnt");
	return utils::readIntegral<SSize>(refcnt);
}


auto RemotePyObject::type() const -> RemotePyTypeObject
{
	return RemotePyTypeObject(baseField("ob_type").GetPtr());
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


auto RemotePyObject::baseField(const string & fieldName) const -> ExtRemoteTyped
{
	ExtRemoteTyped obj = remoteObj();

	// Python3 tucks the base members away in a struct named ob_base.
	while (obj.HasField("ob_base") && !obj.HasField(fieldName.c_str())) {
		// Drill down into the ob_base member until we hit the end.
		obj = obj.Field("ob_base");
	}

	return obj.Field(fieldName.c_str());
}

