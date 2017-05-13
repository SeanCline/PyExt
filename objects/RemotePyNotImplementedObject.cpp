#include "RemotePyNotImplementedObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyNotImplementedObject::RemotePyNotImplementedObject(Offset objectAddress)
	: RemotePyObject(objectAddress)
{
}


string RemotePyNotImplementedObject::repr(bool /*pretty*/) const
{
	return "NotImplemented";
}
