#include "objects.h"
#include "RemotePyObject.h"
#include "RemotePyVarObject.h"
#include "RemotePyStringObject.h"
#include "RemotePyListObject.h"
#include "RemotePyTupleObject.h"
#include "RemotePyDictObject.h"
#include "RemotePyIntObject.h"
#include "RemotePyLongObject.h"
#include "RemotePyFloatObject.h"
#include "RemotePyBoolObject.h"
#include "RemotePyComplexObject.h"
#include "RemotePyNoneObject.h"
#include "RemotePyNotImplementedObject.h"

#include <memory>
using namespace std;


unique_ptr<RemotePyObject> makeRemotePyObject(RemotePyObject::Offset remoteAddress)
{
	// Get the type name.
	const auto obj = RemotePyObject(remoteAddress);
	const auto typeName = obj.getTypeName();

	// TODO: Turn this into a map to factory functions.
	if (typeName == "str") {
		return make_unique<RemotePyStringObject>(remoteAddress);
	} else if (typeName == "list") {
		return make_unique<RemotePyListObject>(remoteAddress);
	} else if (typeName == "tuple") {
		return make_unique<RemotePyTupleObject>(remoteAddress);
	} else if (typeName == "dict") {
		return make_unique<RemotePyDictObject>(remoteAddress);
	} else if (typeName == "int") {
		return make_unique<RemotePyIntObject>(remoteAddress);
	} else if (typeName == "long") {
		return make_unique<RemotePyLongObject>(remoteAddress);
	} else if (typeName == "float") {
		return make_unique<RemotePyFloatObject>(remoteAddress);
	} else if (typeName == "bool") {
		return make_unique<RemotePyBoolObject>(remoteAddress);
	} else if (typeName == "complex") {
		return make_unique<RemotePyComplexObject>(remoteAddress);
	} else if (typeName == "NoneType") {
		return make_unique<RemotePyNoneObject>(remoteAddress);
	} else if (typeName == "NotImplementedType") {
		return make_unique<RemotePyNotImplementedObject>(remoteAddress);
	} else {
		return make_unique<RemotePyObject>(remoteAddress);
	}
}
