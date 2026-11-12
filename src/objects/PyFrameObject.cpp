#include "PyFrameObject.h"


#include "PyCodeObject.h"
#include "PyDictObject.h"
#include "PyFunctionObject.h"
#include "PyInterpreterFrame.h"
#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
#include <sstream>
#include <memory>
using namespace std;

namespace PyExt::Remote {

	PyFrameObject::PyFrameObject(Offset objectAddress)
		: PyObject(objectAddress, "_frame")
	{
	}


	auto PyFrameObject::interpreterFrame() const -> unique_ptr<PyInterpreterFrame>
	{
		if (!remoteType().HasField("f_frame"))
			return nullptr;

		auto frame = remoteType().Field("f_frame");
		if (frame.GetPtr() == 0)
			return nullptr;
		return make_unique<PyInterpreterFrame>(::PyExt::Remote::RemoteType(frame));
	}


	auto PyFrameObject::locals() const -> unique_ptr<PyDictObject>
	{
		if (auto frame = interpreterFrame())
			return frame->locals();

		// Note: The CPython code comments indicate that this isn't always a dict object. In practice, it seems to be.
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "f_locals");
	}

	auto PyFrameObject::localsplus() const -> vector<pair<string, unique_ptr<PyObject>>>
	{
		if (auto frame = interpreterFrame())
			return frame->localsplus();

		auto codeObject = code();
		if (codeObject == nullptr)
			return {};

		vector<string> varNames = codeObject->varNames();
		vector<string> cellVars = codeObject->cellVars();
		vector<string> freeVars = codeObject->freeVars();
		auto numLocalsplus = varNames.size() + cellVars.size() + freeVars.size();
		if (numLocalsplus == 0)
			return {};

		vector<string> names;
		names.reserve(numLocalsplus);
		names.insert(names.end(), varNames.begin(), varNames.end());
		names.insert(names.end(), cellVars.begin(), cellVars.end());
		names.insert(names.end(), freeVars.begin(), freeVars.end());

		auto f_localsplus = remoteType().Field("f_localsplus");
		auto pyObjAddrs = readOffsetArray(f_localsplus, numLocalsplus);
		vector<pair<string, unique_ptr<PyObject>>> localsplus(numLocalsplus);
		for (size_t i = 0; i < numLocalsplus; ++i) {
			auto addr = pyObjAddrs.at(i);
			auto objPtr = addr ? make(addr) : nullptr;
			localsplus[i] = make_pair(names.at(i), move(objPtr));
		}
		return localsplus;
	}

	auto PyFrameObject::globals() const -> unique_ptr<PyDictObject>
	{
		if (auto frame = interpreterFrame())
			return frame->globals();

		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "f_globals");
	}


	auto PyFrameObject::code() const -> unique_ptr<PyCodeObject>
	{
		if (auto frame = interpreterFrame())
			return frame->code();

		return utils::fieldAsPyObject<PyCodeObject>(remoteType(), "f_code");
	}


	auto PyFrameObject::previous() const -> unique_ptr<PyFrame>
	{
		// f_back is only filled in lazily on Python 3.11+, so walk the interpreter frames instead.
		if (auto frame = interpreterFrame())
			return frame->previous();

		return back();
	}


	auto PyFrameObject::back() const -> unique_ptr<PyFrameObject>
	{
		return utils::fieldAsPyObject<PyFrameObject>(remoteType(), "f_back");
	}


	auto PyFrameObject::trace() const -> unique_ptr<PyFunctionObject>
	{
		return utils::fieldAsPyObject<PyFunctionObject>(remoteType(), "f_trace");
	}


	auto PyFrameObject::lastInstruction() const -> int
	{
		auto lasti = remoteType().Field("f_lasti");
		return utils::readIntegral<int>(lasti);
	}


	auto PyFrameObject::currentLineNumber() const -> int
	{
		if (auto frame = interpreterFrame())
			return frame->currentLineNumber();

		// When tracing is enabled, we can use the acccurately updated f_lineno field.
		auto traceFunction = trace();
		auto codeObject = code();
		if (traceFunction != nullptr || codeObject == nullptr) {
			auto lineno = remoteType().Field("f_lineno");
			return utils::readIntegral<int>(lineno);
		}

		// Otherwise, we need to do a lookup into the code object's line number table (co_linetable resp. co_lnotab).
		return codeObject->lineNumberFromInstructionOffset(lastInstruction());
	}


	auto PyFrameObject::repr(bool pretty) const -> string
	{
		return linkToSelf("<frame object>", pretty);
	}


	auto PyFrameObject::details() const -> string
	{
		return PyFrame::details();
	}

}