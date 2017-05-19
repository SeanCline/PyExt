#include "extension.h"

#include "objects/objects.h"
#include "objects/RemotePyObject.h"
#include "objects/RemotePyVarObject.h"
#include "objects/RemotePyTypeObject.h"

#include <engextcpp.hpp>

#include <string>
#include <memory>
using namespace std;


EXT_CLASS::EXT_CLASS()
{
	// Set up our known struct handlers.
	// Windbg uses these to know how to pretty-print types.
	auto handler = static_cast<ExtKnownStructMethod>(&EXT_CLASS::KnownStructObjectHandler);
	static ExtKnownStruct knownstructs[] = {
		{ "PyVarObject",   handler, true },
		{ "PyObject",      handler, true },
		{ "_object",       handler, true },
		{ "PyTypeObject",  handler, true },
		{ "_typeobject",   handler, true },
		{ "PyFrameObject", handler, true },
		{ "_frame",        handler, true },
		{ "_dictobject",   handler, true },
		{ "PyDictObject",  handler, true },
		{ "_setobject",    handler, true },
		{ "PySetObject",   handler, true },
		{ "PyCodeObject",  handler, true },
		{ nullptr,         nullptr, false }
	};

	m_KnownStructs = knownstructs;
}


void EXT_CLASS::KnownStructObjectHandler(_In_ PCSTR /*TypeName*/, _In_ ULONG Flags, _In_ ULONG64 Offset)
{	
	if (Flags == DEBUG_KNOWN_STRUCT_GET_SINGLE_LINE_OUTPUT)
	{
		auto pyObj = makeRemotePyObject(Offset);

		AppendString("Type: %s ", pyObj->type().name().c_str());

		auto repr = pyObj->repr(false);
		if (!repr.empty()) {
			if (repr.size() > m_AppendBufferChars
			  || (ULONG_PTR)(m_AppendAt - m_AppendBuffer) > m_AppendBufferChars - repr.size()) {
				auto message = "<Too long to print. Use !pyobj 0x>"s + to_string(pyObj->offset());
				AppendBufferString(message.c_str());
			} else {
				AppendBufferString(repr.c_str());
			}
		}
	}
}


EXT_COMMAND(pyobj, "Prints information about a Python object", "{;s;PyObject address}")
{
	auto arg0 = GetUnnamedArgStr(0);
	ExtRemoteTyped remoteObj;
	try {
		// First, see if the provided address can be evaluated as-is.
		string objExpression = "(_object*)("s + arg0 + ")"s;
		remoteObj.Set(objExpression.c_str());
	} catch (...) {
		// Otherwise, see if it can be parsed as a number.
		auto offset = EvalExprU64(arg0);
		remoteObj.Set("_object", offset, true);
	}
	
	auto pyObj = makeRemotePyObject(remoteObj.GetPtr());

	Out("%s at address: %y\n", pyObj->symbolName().c_str(), pyObj->offset());
	Out("\tRefCount: %s\n", to_string(pyObj->refCount()).c_str());
	Out("\tType: %s\n", pyObj->type().name().c_str());

	// Print the size if its a PyVarObject.
	auto pyVarObj = dynamic_cast<RemotePyVarObject*>(pyObj.get());
	if (pyVarObj != nullptr)
		Out("\tSize: %d\n", pyVarObj->size());

	auto repr = pyObj->repr(true);
	if (!repr.empty())
		Out("\tRepr: %s\n", repr.c_str());
}
