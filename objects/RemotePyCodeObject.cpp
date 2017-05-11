#include "RemotePyCodeObject.h"
#include "RemotePyStringObject.h"

#include <engextcpp.hpp>
#include <string>

using namespace std;

RemotePyCodeObject::RemotePyCodeObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyCodeObject")
{
}


int RemotePyCodeObject::firstlineno() const
{
	return remoteObj().Field("co_firstlineno").GetLong();
}


string RemotePyCodeObject::filename() const
{
	auto objPtr = remoteObj().Field("co_name").GetPtr();
	if (objPtr == 0)
		return {};
	
	auto filename = RemotePyStringObject(objPtr);
	return filename.stringValue();
}


string RemotePyCodeObject::name() const
{
	auto objPtr = remoteObj().Field("co_name").GetPtr();
	if (objPtr == 0)
		return {};
	
	auto nameObject = RemotePyStringObject(objPtr);
	return nameObject.stringValue();
}


string RemotePyCodeObject::repr(bool pretty) const
{
	return "<code object, file \"" + filename() + "\", line " + to_string(firstlineno()) + ">";
}
