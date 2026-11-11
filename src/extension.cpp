#include "extension.h"

#include "PyObject.h"
#include "PyVarObject.h"
#include "PyTypeObject.h"
#include "PyInterpreterFrame.h"
#include "RemoteType.h"
#include "PyInterpreterState.h"
#include "ExtHelpers.h"
using namespace PyExt::Remote;

#include <engextcpp.hpp>

#include <algorithm>
#include <string>
#include <string_view>
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
					auto message = "<Too long to print. Use !pyobj 0n"s + to_string(pyObj->offset()) + ">";
					AppendBufferString(message.c_str());
				} else {
					AppendBufferString(repr.c_str());
				}
			}
		}
	}


	auto EXT_CLASS::printDml(const string& content) -> void
	{
		// Text read from the debuggee is UTF-8, which the narrow output functions would misread as ANSI.
		const auto wideContent = utils::utf8ToWide(content);
		const wstring_view remaining = wideContent;

		// There is a limit for single output. It seems impossible to know the exact value
		// of the limit in advance, so we take the value from the documentation.
		// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/dbgeng/nf-dbgeng-idebugcontrol4-controlledoutputvalistwide#remarks
		for (size_t chunkOffset = 0; chunkOffset < remaining.size(); ) {
			auto chunkLength = min(chunkSize, remaining.size() - chunkOffset);

			if (chunkOffset + chunkLength < remaining.size()) {
				// Each chunk is parsed as DML on its own, so a tag or entity must not straddle two of them.
				// Our DML never contains a raw newline inside of one, which makes line ends safe places to split.
				auto lastNewline = remaining.substr(chunkOffset, chunkLength).rfind(L'\n');
				if (lastNewline != wstring_view::npos) {
					chunkLength = lastNewline + 1;
				} else if (remaining[chunkOffset + chunkLength - 1] >= 0xD800 && remaining[chunkOffset + chunkLength - 1] <= 0xDBFF) {
					--chunkLength; //< At least keep a surrogate pair together.
				}
			}

			Dml(L"%s", wstring(remaining.substr(chunkOffset, chunkLength)).c_str());
			chunkOffset += chunkLength;
		}
		Out("\n");
	}


	EXT_COMMAND(pyobj, "Prints information about a Python object", "{;s;PyObject address}")
	{
		ensureSymbolsLoaded();

		reportingStdExceptions([&] {
			auto offset = evalOffset(GetUnnamedArgStr(0));
			auto pyObj = PyObject::make(offset);

			Out("%s at address: %y\n", pyObj->symbolName().c_str(), pyObj->offset());
			Out("\tRefCount: %s%s\n", to_string(pyObj->refCount()).c_str(), pyObj->isImmortal() ? " (immortal)" : "");
			Out("\tType: %s\n", pyObj->type().name().c_str());

			// Print the size if its a PyVarObject.
			auto pyVarObj = dynamic_cast<PyVarObject*>(pyObj.get());
			if (pyVarObj != nullptr)
				Out("\tSize: %d\n", pyVarObj->size());

			auto repr = pyObj->repr(true);
			if (!repr.empty()) {
				Out("\tRepr: ");
				printDml(repr);
			}

			auto details = pyObj->details();
			if (!details.empty()) {
				Out("\tDetails:\n");
				printDml(details);
			}
		});
	}


	EXT_COMMAND(pyinterpreterframe, "Prints information about a Python interpreter frame", "{;s;Frame address}")
	{
		ensureSymbolsLoaded();

		reportingStdExceptions([&] {
			auto offset = evalOffset(GetUnnamedArgStr(0));
			auto frame = make_unique<PyInterpreterFrame>(RemoteType(offset, "_PyInterpreterFrame"));

			auto details = frame->details();
			Out("\tDetails:\n");
			printDml(details);
		});
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
