#include "PyGenObject.h"

#include "PyStringValue.h"
#include "PyFrameObject.h"
#include "PyInterpreterFrame.h"
#include "PyFrame.h"
#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <string>
using namespace std;

namespace {

	// Returns the lowercased name of a `_framestate` enumerator without its FRAME_ prefix, e.g. "suspended".
	auto frameStateNameFromSymbols(int state) -> optional<string>
	{
		// Hide the wall of text GetSymbolTypeId prints when the enum isn't in the PDB.
		ExtCaptureOutputA ignoreOut;
		ignoreOut.Start();

		ULONG typeId = 0;
		ULONG64 moduleBase = 0;
		auto& symbols = g_Ext->m_Symbols3;
		if (FAILED(symbols->GetSymbolTypeId(utils::getFullSymbolName("_framestate").c_str(), &typeId, &moduleBase)))
			return nullopt;

		// Negative enumerators may be stored sign-extended or as their 32-bit pattern.
		char name[128] = {};
		if (FAILED(symbols->GetConstantName(moduleBase, typeId, static_cast<ULONG64>(static_cast<LONG64>(state)), name, sizeof(name), nullptr))
			&& FAILED(symbols->GetConstantName(moduleBase, typeId, static_cast<ULONG>(state), name, sizeof(name), nullptr)))
			return nullopt;

		string result = name;
		const string prefix = "FRAME_";
		if (result.starts_with(prefix))
			result.erase(0, prefix.size());
		if (result.empty())
			return nullopt;
		transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
		return result;
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
			return { };

		// The values of `enum _framestate` were renumbered in 3.13 and again in 3.15, so the
		// debuggee's own symbols are the authority on what a value means.
		if (auto name = frameStateNameFromSymbols(*state); name.has_value())
			return *name;

		// Fall back on what Include/internal/pycore_frame.h says for the versions we know about.
		const auto minorVersion = (utils::pythonVersionHex().value_or(0) >> 16) & 0xFF;
		if (minorVersion == 11 || minorVersion == 12) {
			switch (*state) {
			case -2: return "created";
			case -1: return "suspended";
			case 0:  return "executing";
			case 1:  return "completed";
			case 4:  return "cleared";
			}
		} else if (minorVersion == 13 || minorVersion == 14) {
			switch (*state) {
			case -3: return "created";
			case -2: return "suspended";
			case -1: return "suspended_yield_from";
			case 0:  return "executing";
			case 1:  return "completed";
			case 4:  return "cleared";
			}
		}
		return "unknown state " + to_string(*state);
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

		if (pretty)
			return utils::link(repr, "!pyobj 0n"s + to_string(offset()));
		return repr;
	}


	auto PyGenObjectBase::details() const -> string
	{
		ostringstream oss;
		auto qn = qualname();
		if (qn != nullptr)
			oss << "qualname: " << utils::escapeDml(qn->stringValue()) << "\n"; //< Names such as "<genexpr>" would otherwise read as a DML tag.
		if (auto state = frameStateString(); !state.empty())
			oss << "state: " << state << "\n";

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
