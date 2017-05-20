#include "RemotePyNotImplementedObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyNotImplementedObject::RemotePyNotImplementedObject(Offset objectAddress)
	: RemotePyObject(objectAddress)
{
}


auto RemotePyNotImplementedObject::repr(bool /*pretty*/) const -> string
{
	return "NotImplemented";
}
