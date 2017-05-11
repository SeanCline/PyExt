#include "extension.h"

#include "objects/objects.h"

#include <engextcpp.hpp>

#include <string>
using namespace std;


EXT_CLASS::EXT_CLASS()
{
	// Set up our known struct handlers.
	// Windbg uses these to know how to pretty-print types.
	auto handler = static_cast<ExtKnownStructMethod>(&EXT_CLASS::KnownStructObjectHandler);
	static ExtKnownStruct knownstructs[] = {
		{ "PyObject",      handler, true },
		{ "_object",       handler, true },
		{ "_frame",        handler, true },
		{ "PyFrameObject", handler, true },
		{ "PyCodeObject",  handler, true },
		{ "_typeobject",   handler, true },
		{ nullptr, nullptr, false }
	};

	m_KnownStructs = knownstructs;
}


void EXT_CLASS::KnownStructObjectHandler(_In_ PCSTR TypeName, _In_ ULONG Flags, _In_ ULONG64 Offset)
{	
	if (Flags == DEBUG_KNOWN_STRUCT_GET_SINGLE_LINE_OUTPUT)
	{
		auto pyObj = makeRemotePyObject(Offset);

		AppendString("Type: %s ", pyObj->getTypeName().c_str());

		auto repr = pyObj->repr(false);
		if (!repr.empty()) {
			if (repr.size() > m_AppendBufferChars
			  || (ULONG_PTR)(m_AppendAt - m_AppendBuffer) > m_AppendBufferChars - repr.size()) {
				AppendBufferString("<Too long to print. Use !pyobj>");
			} else {
				AppendBufferString(repr.c_str());
			}
		}
	}
}


EXT_COMMAND(pyobj, "Prints information about a Python object", "{;s;PyObject address}")
{
	string objExpression = "(_object*)(" + string(GetUnnamedArgStr(0)) + ")";
	ExtRemoteTyped parsedObj(objExpression.c_str());
	auto pyObj = makeRemotePyObject(parsedObj.GetPtr());

	Out("RefCount: %s\n", to_string(pyObj->getRefCount()).c_str());
	Out("Type: %s\n", pyObj->getTypeName().c_str());

	auto repr = pyObj->repr(true);
	if (!repr.empty()) {
		Out("Repr: %s\n", repr.c_str());
	}
}
