#include "extension.h"

#include "PyObject.h"
#include "PyVarObject.h"
#include "PyTypeObject.h"
#include "PyInterpreterFrame.h"
#include "RemoteType.h"
#include "PyInterpreterState.h"
using namespace PyExt::Remote;

#include <engextcpp.hpp>

#include <string>
#include <memory>
using namespace std;

namespace PyExt {

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
			auto pyObj = PyObject::make(Offset);

			AppendString("Type: %s ", pyObj->type().name().c_str());

			auto repr = pyObj->repr(false);
			if (!repr.empty()) {
				if (repr.size() > m_AppendBufferChars
					|| (ULONG_PTR)(m_AppendAt - m_AppendBuffer) > m_AppendBufferChars - repr.size()) {
					auto message = "<Too long to print. Use !pyobj 0n"s + to_string(pyObj->offset());
					AppendBufferString(message.c_str());
				} else {
					AppendBufferString(repr.c_str());
				}
			}
		}
	}


	auto EXT_CLASS::printDml(const char* format, const string& content) -> void
	{
		// There is a limit for single output. It seems impossible to know the exact value
		// of the limit in advance, so we take the value from the documentation.
		// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugcontrol4-controlledoutputvalistwide#remarks

		if (content.size() <= chunkSize) {
			Dml(format, content.c_str());
			return;
		}

		for (size_t chunkOffset = 0; chunkOffset < content.size(); chunkOffset += chunkSize) {
			Dml(format, content.substr(chunkOffset, chunkSize).c_str());
			format = "%s";
		}
	}


	EXT_COMMAND(pyobj, "Prints information about a Python object", "{;s;PyObject address}")
	{
		ensureSymbolsLoaded();

		auto offset = evalOffset(GetUnnamedArgStr(0));
		auto pyObj = PyObject::make(offset);

		Out("%s at address: %y\n", pyObj->symbolName().c_str(), pyObj->offset());
		Out("\tRefCount: %s\n", to_string(pyObj->refCount()).c_str());
		Out("\tType: %s\n", pyObj->type().name().c_str());

		// Print the size if its a PyVarObject.
		auto pyVarObj = dynamic_cast<PyVarObject*>(pyObj.get());
		if (pyVarObj != nullptr)
			Out("\tSize: %d\n", pyVarObj->size());

		auto repr = pyObj->repr(true);
		if (!repr.empty())
			printDml("\tRepr: %s\n", repr);

		auto details = pyObj->details();
		if (!details.empty())
			printDml("\tDetails:\n%s\n", details);
	}


	EXT_COMMAND(pyinterpreterframe, "Prints information about a Python interpreter frame", "{;s;Frame address}")
	{
		ensureSymbolsLoaded();

		auto offset = evalOffset(GetUnnamedArgStr(0));
		auto frame = make_unique<PyInterpreterFrame>(RemoteType(offset, "_PyInterpreterFrame"));

		auto details = frame->details();
		printDml("\tDetails:\n%s\n", details);
	}


	auto EXT_CLASS::ensureSymbolsLoaded() -> void
	{
		// Hide the massive wall o'text `GetSymbolTypeId` prints when it fails to resolve a symbol.
		// We'll print our own if it fails...
		ExtCaptureOutputA ignoreOut;
		ignoreOut.Start();

		// See if the symbol can be found.
		try {
			// If we can construct the AutoInterpreterState, assume symbols are good.
			PyInterpreterState::makeAutoInterpreterState();
			return;
		} catch (...) { }

		// See if triggering a reload and retrying helps matters.
		m_Symbols->Reload("/f python*");

		// Try again now that symbols are reloaded.
		try {
			// If we can construct the AutoInterpreterState, assume symbols are good.
			PyInterpreterState::makeAutoInterpreterState();
			return;
		}
		catch (...) { }

		// No luck finding the interpreter state symbol. Print a message to the user.

		ignoreOut.Delete();
		Err("\n\n");
		Err("*************************************************************************\n");
		Err("***            ERROR: Python symbols could not be loaded.             ***\n");
		Err("***   Install the debugging symbols for your version of Python##.dll  ***\n");
		Err("***                 and add them to the symbol path.                  ***\n");
		Err("***                                                                   ***\n");
		Err("***   Alternatively, you run the command !pysymfix to add the Python  ***\n");
		Err("***        symbol server created for use with this extension          ***\n");
		Err("*************************************************************************\n");
		Err("\n\n");
		// Don't bother throwing. If there really is a symbol issue, it will be caught later on.
	}


	auto EXT_CLASS::evalOffset(const string& arg) -> UINT64
	{
		// First, see if the arg can be parsed as an expression.
		try {
			string objExpression = "(void*)("s + arg + ")"s;
			ExtRemoteTyped remoteObj(objExpression.c_str());
			return remoteObj.GetPtr();
		} catch (ExtException&) {
			// Fall back on evaluating it as a number.
			return g_Ext->EvalExprU64(arg.c_str());
		}
	}

}
