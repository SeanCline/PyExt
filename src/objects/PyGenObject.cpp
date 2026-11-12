#include "PyGenObject.h"

#include "PyStringValue.h"
#include "PyFrameObject.h"
#include "PyInterpreterFrame.h"
#include "PyFrame.h"
#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"
#include "../PyLayout.h"
#include "../ReprBuilder.h"

#include <engextcpp.hpp>

#include <cctype>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
using namespace std;

namespace {

	// Names a value of CPython's `enum _framestate` the way the rest of PyExt's output is written:
	// "FRAME_SUSPENDED" becomes "suspended".
	//
	// The enum's values were renumbered in Python 3.13 and again in 3.15, so the name is looked up
	// in the debuggee's symbols rather than in a table of our own that would go stale.
	auto frameStateName(int state) -> string
	{
		auto name = PyExt::Layout::enumeratorName("_framestate", state);
		if (!name.has_value() || !name->starts_with("FRAME_"))
			return "unknown state " + to_string(state);

		name->erase(0, strlen("FRAME_"));
		for (auto& c : *name)
			c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		return *name;
	}

}

namespace PyExt::Remote {

	PyGenObjectBase::PyGenObjectBase(Offset objectAddress, const string& symbolName, const string& fieldPrefix, const string& displayName)
		: PyObject(objectAddress, symbolName),
		  fieldPrefix_(fieldPrefix),
		  displayName_(displayName)
	{
	}


	auto PyGenObjectBase::prefixedField(const string& suffix) const -> string
	{
		return fieldPrefix_ + "_" + suffix;
	}


	auto PyGenObjectBase::name() const -> unique_ptr<PyStringValue>
	{
		return utils::fieldAsPyObject<PyStringValue>(remoteType(), prefixedField("name"));
	}


	auto PyGenObjectBase::qualname() const -> unique_ptr<PyStringValue>
	{
		return utils::fieldAsPyObject<PyStringValue>(remoteType(), prefixedField("qualname"));
	}


	auto PyGenObjectBase::frameState() const -> optional<int>
	{
		// Before Python 3.11 the state lived on the generator's PyFrameObject instead.
		auto fieldName = prefixedField("frame_state");
		if (!remoteType().HasField(fieldName.c_str()))
			return nullopt;
		auto fs = remoteType().Field(fieldName.c_str());
		return utils::readIntegral<int>(fs);
	}


	auto PyGenObjectBase::frameStateString() const -> string
	{
		auto state = frameState();
		if (!state.has_value())
			return legacyFrameStateString();
		return frameStateName(*state);
	}


	auto PyGenObjectBase::legacyFrameStateString() const -> string
	{
		auto frameFieldName = prefixedField("frame");
		if (!remoteType().HasField(frameFieldName.c_str()))
			return { };

		// The generator lets go of its frame once it has finished.
		auto frameField = remoteType().Field(frameFieldName.c_str());
		if (frameField.GetPtr() == 0)
			return "cleared";

		// Python 3.10 keeps the state on the frame object.
		if (frameField.HasField("f_state")) {
			auto stateField = frameField.Field("f_state");
			return frameStateName(utils::readIntegral<int>(stateField));
		}

		// Before that there is only a running flag, and the frame's last instruction to say whether it has started.
		auto runningFieldName = prefixedField("running");
		if (remoteType().HasField(runningFieldName.c_str())) {
			auto runningField = remoteType().Field(runningFieldName.c_str());
			if (utils::readIntegral<int>(runningField) != 0)
				return "executing";
		}

		auto lastInstruction = frameField.Field("f_lasti");
		return utils::readIntegral<int>(lastInstruction) < 0 ? "created" : "suspended";
	}


	auto PyGenObjectBase::frame() const -> unique_ptr<PyFrame>
	{
		// Python 3.11+: an embedded _PyInterpreterFrame named gi_iframe / cr_iframe / ag_iframe.
		// We rebuild the frame from its address + symbol name so that callers of offset() get
		// the embedded struct's address rather than a struct-typed GetPtr() (which throws).
		auto iframeName = prefixedField("iframe");
		if (remoteType().HasField(iframeName.c_str())) {
			auto iframeAddr = remoteType().Field(iframeName.c_str()).GetPointerTo().GetPtr();
			return make_unique<PyInterpreterFrame>(RemoteType(iframeAddr, "_PyInterpreterFrame"));
		}

		// Pre-3.11: a separate PyFrameObject pointer named gi_frame / cr_frame / ag_frame.
		auto framePtrName = prefixedField("frame");
		auto framePtr = utils::fieldAsPyObject<PyFrameObject>(remoteType(), framePtrName);
		if (framePtr != nullptr)
			return framePtr;

		return nullptr;
	}


	auto PyGenObjectBase::repr(bool pretty) const -> string
	{
		string nameStr;
		auto qn = qualname();
		if (qn != nullptr) {
			nameStr = qn->stringValue();
		} else if (auto n = name(); n != nullptr) {
			nameStr = n->stringValue();
		}

		string repr = "<" + displayName_ + " object";
		if (!nameStr.empty())
			repr += " " + nameStr;
		if (auto state = frameStateString(); !state.empty())
			repr += " (" + state + ")";
		repr += ">";

		return linkToSelf(repr, pretty);
	}


	auto PyGenObjectBase::details() const -> string
	{
		utils::ReprBuilder out(true); //< Details are always printed as DML.
		auto qn = qualname();
		if (qn != nullptr)
			out.text("qualname: " + qn->stringValue() + "\n");
		if (auto state = frameStateString(); !state.empty())
			out.text("state: " + state + "\n");

		auto f = frame();
		if (f != nullptr)
			out.markup(f->details());
		return out.str();
	}


	PyGenObject::PyGenObject(Offset objectAddress)
		: PyGenObjectBase(objectAddress, "PyGenObject", "gi", "generator")
	{
	}


	PyCoroObject::PyCoroObject(Offset objectAddress)
		: PyGenObjectBase(objectAddress, "PyCoroObject", "cr", "coroutine")
	{
	}


	PyAsyncGenObject::PyAsyncGenObject(Offset objectAddress)
		: PyGenObjectBase(objectAddress, "PyAsyncGenObject", "ag", "async_generator")
	{
	}

}
