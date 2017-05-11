#include "RemotePyFrameObject.h"

#include "RemotePyCodeObject.h"
#include "RemotePyDictObject.h"

#include <engextcpp.hpp>
#include <string>
#include <memory>
using namespace std;

RemotePyFrameObject::RemotePyFrameObject(Offset objectAddress)
	: RemotePyVarObject(objectAddress, "PyFrameObject")
{
}


unique_ptr<RemotePyDictObject> RemotePyFrameObject::locals() const
{
	// Note: The CPython code comments indicate that this isn't always a dict object. In practice, it seems to be.
	auto ptr = remoteObj().Field("f_locals").GetPtr();
	if (ptr == 0)
		return {};

	return make_unique<RemotePyDictObject>(ptr);
}


unique_ptr<RemotePyDictObject> RemotePyFrameObject::globals() const
{
	auto ptr = remoteObj().Field("f_globals").GetPtr();
	if (ptr == 0)
		return {};

	return make_unique<RemotePyDictObject>(ptr);
}


unique_ptr<RemotePyDictObject> RemotePyFrameObject::builtins() const
{
	auto ptr = remoteObj().Field("f_builtins").GetPtr();
	if (ptr == 0)
		return {};

	return make_unique<RemotePyDictObject>(ptr);
}


unique_ptr<RemotePyCodeObject> RemotePyFrameObject::code() const
{
	auto ptr = remoteObj().Field("f_code").GetPtr();
	if (ptr == 0)
		return {};

	return make_unique<RemotePyCodeObject>(ptr);
}


unique_ptr<RemotePyFrameObject> RemotePyFrameObject::back() const
{
	auto ptr = remoteObj().Field("f_back").GetPtr();
	if (ptr == 0)
		return {};

	return make_unique<RemotePyFrameObject>(ptr);
}


string RemotePyFrameObject::repr(bool pretty) const
{
	return "<frame object>";
}
