#include "RemotePyNoneObject.h"

#include <engextcpp.hpp>
#include <string>
using namespace std;

RemotePyNoneObject::RemotePyNoneObject(Offset objectAddress)
	: RemotePyObject(objectAddress)
{
}


string RemotePyNoneObject::repr(bool /*pretty*/) const
{
	return "None";
}
