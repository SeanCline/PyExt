#include "PyFrameObject.h"


#include "PyCodeObject.h"
#include "PyDictObject.h"
#include "PyFunctionObject.h"
#include "../fieldAsPyObject.h"
#include "../ExtHelpers.h"

#include <engextcpp.hpp>
#include <string>
#include <sstream>
#include <memory>
using namespace std;

namespace PyExt::Remote {

	PyFrameObject::PyFrameObject(Offset objectAddress)
		: PyVarObject(objectAddress, "PyFrameObject")
	{
	}


	auto PyFrameObject::locals() const -> unique_ptr<PyDictObject>
	{
		// Note: The CPython code comments indicate that this isn't always a dict object. In practice, it seems to be.
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "f_locals");
	}

	auto PyFrameObject::localsplus() const -> vector<pair<string, unique_ptr<PyObject>>>
	{
		auto codeObject = code();
		if (codeObject == nullptr)
			return {};
		// TODO: co_cellvars + co_freevars
		int numLocalsPlus = codeObject->numberOfLocals();
		if (numLocalsPlus == 0)
			return {};
		vector<string> varNames = codeObject->varNames();
		auto f_localsplus = remoteType().Field("f_localsplus");
		auto pyObjAddrs = utils::readArray<PyObject::Offset>(f_localsplus, numLocalsPlus);
		vector<pair<string, unique_ptr<PyObject>>> localsplus(numLocalsPlus);
		for (size_t i = 0; i < numLocalsPlus; ++i) {
			auto addr = pyObjAddrs.at(i);
			auto objPtr = PyObject::make(addr);
			localsplus[i] = make_pair(varNames.at(i), move(objPtr));
		}
		return localsplus;
	}

	auto PyFrameObject::globals() const -> unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "f_globals");
	}


	auto PyFrameObject::builtins() const -> unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "f_builtins");
	}


	auto PyFrameObject::code() const -> unique_ptr<PyCodeObject>
	{
		return utils::fieldAsPyObject<PyCodeObject>(remoteType(), "f_code");
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
		// When tracing is enabled, we can use the acccurately updated f_lineno field.
		auto traceFunction = trace();
		auto codeObject = code();
		if (traceFunction != nullptr || codeObject == nullptr) {
			auto lineno = remoteType().Field("f_lineno");
			return utils::readIntegral<int>(lineno);
		}

		// Otherwise, we need to do a lookup into the code object's line number table (co_lnotab).
		return codeObject->lineNumberFromInstructionOffset(lastInstruction());
	}


	auto PyFrameObject::repr(bool pretty) const -> string
	{
		string repr = "<frame object>";
		if (pretty)
			return utils::link(repr, "!pyobj 0n"s + to_string(offset()));
		return repr;
	}


	auto PyFrameObject::details() const -> string
	{
		const auto elementSeparator = "\n";
		const auto indentation = "\t";

		ostringstream oss;
		oss << "localsplus: {" << elementSeparator;

		for (auto const& pairValue : localsplus()) {
			auto const& key = pairValue.first;
			auto const& value = pairValue.second;
			oss << indentation << key << ": " << value->repr(true) << ',' << elementSeparator;
		}

		oss << '}';
		return oss.str();
	}

}