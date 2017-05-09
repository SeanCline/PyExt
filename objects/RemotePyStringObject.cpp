#include "RemotePyStringObject.h"

#include "util/to_string.h"
#include <engextcpp.hpp>
#include <string>
#include <sstream>
#include <iomanip>
using namespace std;

RemotePyStringObject::RemotePyStringObject(Offset objectAddress)
	: RemotePyVarObject(objectAddress, "PyStringObject")
{
}


std::string RemotePyStringObject::stringValue() const
{
	auto sval = remoteObj().Field("ob_sval");
	ExtBuffer<char> buff;
	sval.Dereference().GetString(&buff);
	return to_string(buff);
}


string RemotePyStringObject::repr(bool pretty) const
{
	ostringstream oss;
	oss << quoted(stringValue()); //< Escape the string.
	return oss.str();
}
