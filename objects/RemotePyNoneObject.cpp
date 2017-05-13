#include "RemotePyNoneObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyNoneObject::RemotePyNoneObject(Offset objectAddress)
	: RemotePyObject(objectAddress)
{
}


auto RemotePyNoneObject::repr(bool /*pretty*/) const -> string
{
	return "None";
}
