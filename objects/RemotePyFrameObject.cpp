#include "RemotePyFrameObject.h"

#include "utils/fieldAsPyObject.h"
#include "utils/ExtHelpers.h"
#include "RemotePyCodeObject.h"
#include "RemotePyDictObject.h"
#include "RemotePyFunctionObject.h"

#include <engextcpp.hpp>
#include <string>
#include <memory>
using namespace std;

RemotePyFrameObject::RemotePyFrameObject(Offset objectAddress)
	: RemotePyVarObject(objectAddress, "PyFrameObject")
{
}


auto RemotePyFrameObject::locals() const -> unique_ptr<RemotePyDictObject>
{
	// Note: The CPython code comments indicate that this isn't always a dict object. In practice, it seems to be.
	return fieldAsPyObject<RemotePyDictObject>(remoteObj(), "f_locals");
}


auto RemotePyFrameObject::globals() const -> unique_ptr<RemotePyDictObject>
{
	return fieldAsPyObject<RemotePyDictObject>(remoteObj(), "f_globals");
}


auto RemotePyFrameObject::builtins() const -> unique_ptr<RemotePyDictObject>
{
	return fieldAsPyObject<RemotePyDictObject>(remoteObj(), "f_builtins");
}


auto RemotePyFrameObject::code() const -> unique_ptr<RemotePyCodeObject>
{
	return fieldAsPyObject<RemotePyCodeObject>(remoteObj(), "f_code");
}


auto RemotePyFrameObject::back() const -> unique_ptr<RemotePyFrameObject>
{
	return fieldAsPyObject<RemotePyFrameObject>(remoteObj(), "f_back");
}


auto RemotePyFrameObject::trace() const -> unique_ptr<RemotePyFunctionObject>
{
	return fieldAsPyObject<RemotePyFunctionObject>(remoteObj(), "f_trace");
}


auto RemotePyFrameObject::lastInstruction() const -> int
{
	auto lasti = remoteObj().Field("f_lasti");
	return readIntegral<int>(lasti);
}


auto RemotePyFrameObject::currentLineNumber() const -> int
{
	// When tracing is enabled, we can use the acccurately updated f_lineno field.
	auto traceFunction = trace();
	auto codeObject = code();
	if (traceFunction != nullptr || codeObject == nullptr) {
		auto lineno = remoteObj().Field("f_lineno");
		return readIntegral<int>(lineno);
	}

	// Otherwise, we need to do a lookup into the code object's line number table (co_lnotab).
	return codeObject->lineNumberFromInstructionOffset(lastInstruction());
}


auto RemotePyFrameObject::repr(bool /*pretty*/) const -> string
{
	return "<frame object>";
}
