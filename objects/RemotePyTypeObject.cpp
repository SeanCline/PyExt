#include "RemotePyTypeObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyTypeObject::RemotePyTypeObject(Offset objectAddress)
	: RemotePyVarObject(objectAddress, "PyTypeObject")
{
}


auto RemotePyTypeObject::name() const -> string
{
	ExtBuffer<char> buff;
	remoteObj().Field("tp_name").Dereference().GetString(&buff);
	return buff.GetBuffer();
}


auto RemotePyTypeObject::documentation() const -> string
{
	ExtBuffer<char> buff;
	auto doc = remoteObj().Field("ob_type").Field("tp_doc");
	if (doc.GetPtr() == 0)
		return {};

	doc.Dereference().GetString(&buff);
	return buff.GetBuffer();
}


auto RemotePyTypeObject::repr(bool /*pretty*/) const -> string
{
	return "<class '" + name() + "'>";
}
