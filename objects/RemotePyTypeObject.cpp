#include "RemotePyTypeObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyTypeObject::RemotePyTypeObject(Offset objectAddress)
	: RemotePyVarObject(objectAddress, "PyTypeObject")
{
}


std::string RemotePyTypeObject::name() const
{
	ExtBuffer<char> buff;
	remoteObj().Field("tp_name").Dereference().GetString(&buff);
	return buff.GetBuffer();
}


std::string RemotePyTypeObject::documentation() const
{
	ExtBuffer<char> buff;
	auto doc = remoteObj().Field("ob_type").Field("tp_doc");
	if (doc.GetPtr() == 0)
		return {};

	doc.Dereference().GetString(&buff);
	return buff.GetBuffer();
}


string RemotePyTypeObject::repr(bool /*pretty*/) const
{
	return "<class '" + name() + "'>";
}
