#include "PyInterpreterFrame.h"


#include "PyObject.h"
#include "PyCodeObject.h"
#include "PyDictObject.h"
#include "PyFrameObject.h"

#include "fieldAsPyObject.h"
#include "ExtHelpers.h"

#include <engextcpp.hpp>

#include <memory>
using namespace std;

namespace PyExt::Remote {

	PyInterpreterFrame::PyInterpreterFrame(const RemoteType& remoteType)
		: RemoteType(remoteType)
	{
	}


	auto PyInterpreterFrame::locals() const -> unique_ptr<PyDictObject>
	{
		// Note: The CPython code comments indicate that this isn't always a dict object. In practice, it seems to be.
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "f_locals");
	}

	auto PyInterpreterFrame::localsplus() const -> vector<pair<string, unique_ptr<PyObject>>>
	{
		auto codeObject = code();
		if (codeObject == nullptr)
			return {};

		vector<string> names = codeObject->localsplusNames();
		auto numLocalsplus = names.size();
		if (numLocalsplus == 0)
			return {};

		auto f_localsplus = remoteType().Field("localsplus");
		auto pyObjAddrs = readOffsetArray(f_localsplus, numLocalsplus);
		vector<pair<string, unique_ptr<PyObject>>> localsplus(numLocalsplus);
		for (size_t i = 0; i < numLocalsplus; ++i) {
			auto addr = pyObjAddrs.at(i);
			auto objPtr = addr ? PyObject::make(addr) : nullptr;
			localsplus[i] = make_pair(names.at(i), move(objPtr));
		}
		return localsplus;
	}

	auto PyInterpreterFrame::globals() const -> unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "f_globals");
	}


	auto PyInterpreterFrame::code() const -> unique_ptr<PyCodeObject>
	{
		return utils::fieldAsPyObject<PyCodeObject>(remoteType(), "f_code");
	}


	auto PyInterpreterFrame::previous() const -> unique_ptr<PyFrame>
	{
		auto previous = remoteType().Field("previous");
		if (previous.GetPtr() == 0)
			return { };

		return make_unique<PyInterpreterFrame>(RemoteType(previous));
	}


	auto PyInterpreterFrame::prevInstruction() const -> int
	{
		auto prevInstr = remoteType().Field("prev_instr");
		return utils::readIntegral<int>(prevInstr);
	}


	auto PyInterpreterFrame::currentLineNumber() const -> int
	{
		auto codeObject = code();

		// Do a lookup into the code object's line number table (co_linetable).
		return codeObject->lineNumberFromPrevInstruction(prevInstruction());
	}
}
