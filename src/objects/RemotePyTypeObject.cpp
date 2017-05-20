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


// This isn't really the best place for this method, but it's useful for factories to know
// which type to construct when a Python3 type differs from a Python 2 type of the same name.
auto RemotePyTypeObject::isPython2() const -> bool
{
	return !remoteObj().HasField("ob_base");
}


auto RemotePyTypeObject::repr(bool /*pretty*/) const -> string
{
	return "<class '" + name() + "'>";
}
