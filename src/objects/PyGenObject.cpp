#include "PyGenObject.h"

#include "PyStringValue.h"
#include "PyFrameObject.h"
#include "PyInterpreterFrame.h"
#include "PyFrame.h"
#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <sstream>
#include <string>
using namespace std;

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


	auto PyGenObjectBase::frameState() const -> int
	{
		auto fieldName = prefixedField("frame_state");
		if (!remoteType().HasField(fieldName.c_str()))
			return 0;
		auto fs = remoteType().Field(fieldName.c_str());
		return utils::readIntegral<int>(fs);
	}


	auto PyGenObjectBase::frameStateString() const -> string
	{
		// Mirrors Include/internal/pycore_frame.h.
		// Values present from Python 3.11+.
		switch (frameState()) {
		case -3: return "created";
		case -2: return "suspended";
		case -1: return "suspended_yield_from";
		case 0:  return "executing";
		case 1:  return "completed";
		case 4:  return "cleared";
		default: return "unknown";
		}
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
		repr += " (" + frameStateString() + ")>";

		if (pretty)
			return utils::link(repr, "!pyobj 0n"s + to_string(offset()));
		return repr;
	}


	auto PyGenObjectBase::details() const -> string
	{
		ostringstream oss;
		auto qn = qualname();
		if (qn != nullptr)
			oss << "qualname: " << qn->stringValue() << "\n";
		oss << "state: " << frameStateString() << "\n";

		auto f = frame();
		if (f != nullptr)
			oss << f->details();
		return oss.str();
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
