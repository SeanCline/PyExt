#include "RemotePyFunctionObject.h"

#include "RemotePyCodeObject.h"
#include "RemotePyDictObject.h"
#include "RemotePyTupleObject.h"
#include "RemotePyDictObject.h"
#include "RemotePyListObject.h"
#include "RemotePyStringObject.h"
#include "objects/objects.h"
#include "utils/fieldAsPyObject.h"

#include <engextcpp.hpp>

#include <string>
#include <memory>
using namespace std;


RemotePyFunctionObject::RemotePyFunctionObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyFunctionObject")
{
}


auto RemotePyFunctionObject::code() const -> unique_ptr<RemotePyCodeObject>
{
	return utils::fieldAsPyObject<RemotePyCodeObject>(remoteObj(), "func_code");
}


auto RemotePyFunctionObject::globals() const -> unique_ptr<RemotePyDictObject>
{
	return utils::fieldAsPyObject<RemotePyDictObject>(remoteObj(), "func_globals");
}


auto RemotePyFunctionObject::defaults() const -> unique_ptr<RemotePyTupleObject>
{
	return utils::fieldAsPyObject<RemotePyTupleObject>(remoteObj(), "func_defaults");
}


auto RemotePyFunctionObject::kwdefaults() const -> unique_ptr<RemotePyDictObject>
{
	return utils::fieldAsPyObject<RemotePyDictObject>(remoteObj(), "func_kwdefaults");
}


auto RemotePyFunctionObject::closure() const -> unique_ptr<RemotePyTupleObject>
{
	return utils::fieldAsPyObject<RemotePyTupleObject>(remoteObj(), "func_closure");
}


auto RemotePyFunctionObject::doc() const -> unique_ptr<RemotePyObject>
{
	return utils::fieldAsPyObject<RemotePyObject>(remoteObj(), "func_doc");
}


auto RemotePyFunctionObject::name() const -> unique_ptr<RemotePyStringObject>
{
	return utils::fieldAsPyObject<RemotePyStringObject>(remoteObj(), "func_name");
}


auto RemotePyFunctionObject::dict() const -> unique_ptr<RemotePyDictObject>
{
	return utils::fieldAsPyObject<RemotePyDictObject>(remoteObj(), "func_dict");
}


auto RemotePyFunctionObject::weakreflist() const -> unique_ptr<RemotePyListObject>
{
	return utils::fieldAsPyObject<RemotePyListObject>(remoteObj(), "func_weakreflist");
}


auto RemotePyFunctionObject::module() const -> unique_ptr<RemotePyObject>
{
	return utils::fieldAsPyObject<RemotePyObject>(remoteObj(), "func_module");
}


auto RemotePyFunctionObject::annotations() const -> unique_ptr<RemotePyDictObject>
{
	return utils::fieldAsPyObject<RemotePyDictObject>(remoteObj(), "func_annotations");
}


auto RemotePyFunctionObject::qualname() const -> unique_ptr<RemotePyStringObject>
{
	return utils::fieldAsPyObject<RemotePyStringObject>(remoteObj(), "func_qualname");
}


auto RemotePyFunctionObject::repr(bool /*pretty*/) const -> string
{
	return "<function " + name()->stringValue() + ">";
}
