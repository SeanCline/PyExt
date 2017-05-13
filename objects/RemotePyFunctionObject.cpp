#include "RemotePyFunctionObject.h"

#include "objects/objects.h"

#include "RemotePyCodeObject.h"
#include "RemotePyDictObject.h"
#include "RemotePyTupleObject.h"
#include "RemotePyDictObject.h"
#include "RemotePyListObject.h"
#include "RemotePyStringObject.h"

#include <engextcpp.hpp>
#include <string>
#include <memory>
using namespace std;

namespace {
	template <typename T>
	std::unique_ptr<T> fieldAsPyObject(ExtRemoteTyped& remoteType, const string& fieldName)
	{
		if (!remoteType.HasField(fieldName.c_str()))
			return { };

		auto fieldPtr = remoteType.Field(fieldName.c_str()).GetPtr();
		if (fieldPtr == 0)
			return { };

		auto objPtr = makeRemotePyObject(fieldPtr);
		auto derivedObjPtr = dynamic_cast<T*>(objPtr.get());

		// If the dynamic_cast worked, transfer ownership.
		if (derivedObjPtr != nullptr)
			objPtr.release();

		return std::unique_ptr<T>(derivedObjPtr);
	}
}


RemotePyFunctionObject::RemotePyFunctionObject(Offset objectAddress)
	: RemotePyObject(objectAddress, "PyFunctionObject")
{
}


auto RemotePyFunctionObject::code() const -> std::unique_ptr<RemotePyCodeObject>
{
	return fieldAsPyObject<RemotePyCodeObject>(remoteObj(), "func_code");
}


auto RemotePyFunctionObject::globals() const -> std::unique_ptr<RemotePyDictObject>
{
	return fieldAsPyObject<RemotePyDictObject>(remoteObj(), "func_globals");
}


auto RemotePyFunctionObject::defaults() const -> std::unique_ptr<RemotePyTupleObject>
{
	return fieldAsPyObject<RemotePyTupleObject>(remoteObj(), "func_defaults");
}


auto RemotePyFunctionObject::kwdefaults() const -> std::unique_ptr<RemotePyDictObject>
{
	return fieldAsPyObject<RemotePyDictObject>(remoteObj(), "func_kwdefaults");
}


auto RemotePyFunctionObject::closure() const -> std::unique_ptr<RemotePyTupleObject>
{
	return fieldAsPyObject<RemotePyTupleObject>(remoteObj(), "func_closure");
}


auto RemotePyFunctionObject::doc() const -> std::unique_ptr<RemotePyObject>
{
	return fieldAsPyObject<RemotePyObject>(remoteObj(), "func_doc");
}


auto RemotePyFunctionObject::name() const -> std::unique_ptr<RemotePyStringObject>
{
	return fieldAsPyObject<RemotePyStringObject>(remoteObj(), "func_name");
}


auto RemotePyFunctionObject::dict() const -> std::unique_ptr<RemotePyDictObject>
{
	return fieldAsPyObject<RemotePyDictObject>(remoteObj(), "func_dict");
}


auto RemotePyFunctionObject::weakreflist() const -> std::unique_ptr<RemotePyListObject>
{
	return fieldAsPyObject<RemotePyListObject>(remoteObj(), "func_weakreflist");
}


auto RemotePyFunctionObject::module() const -> std::unique_ptr<RemotePyObject>
{
	return fieldAsPyObject<RemotePyObject>(remoteObj(), "func_module");
}


auto RemotePyFunctionObject::annotations() const -> std::unique_ptr<RemotePyDictObject>
{
	return fieldAsPyObject<RemotePyDictObject>(remoteObj(), "func_annotations");
}


auto RemotePyFunctionObject::qualname() const -> std::unique_ptr<RemotePyStringObject>
{
	return fieldAsPyObject<RemotePyStringObject>(remoteObj(), "func_qualname");
}


string RemotePyFunctionObject::repr(bool /*pretty*/) const
{
	return "<function " + name()->stringValue() + ">";
}
