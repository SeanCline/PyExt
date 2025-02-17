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
		auto code = utils::fieldAsPyObject<PyCodeObject>(remoteType(), "f_executable");
		if (code != nullptr)
			return code;  // Python 3.13+
		return utils::fieldAsPyObject<PyCodeObject>(remoteType(), "f_code");
	}


	auto PyInterpreterFrame::previous() const -> unique_ptr<PyFrame>
	{
		auto previous = remoteType().Field("previous");
		if (previous.GetPtr() == 0)
			return { };

		auto ownerRaw = previous.Field("owner");
		auto owner = utils::readIntegral<int8_t>(ownerRaw);
		if (owner == 3)  { // FRAME_OWNED_BY_CSTACK
			// see https://github.com/python/cpython/blob/3bd942f106aa36c261a2d90104c027026b2a8fb6/Python/traceback.c#L979-L982
			previous = previous.Field("previous");
			if (previous.GetPtr() == 0)
				return { };

			ownerRaw = previous.Field("owner");
			owner = utils::readIntegral<int8_t>(ownerRaw);
			if (owner == 3)
				throw runtime_error("Cannot have more than one shim frame in a row.");
		}

		return make_unique<PyInterpreterFrame>(RemoteType(previous));
	}


	auto PyInterpreterFrame::prevInstruction() const -> int
	{
		auto instrPtr = remoteType().HasField("instr_ptr")
			? remoteType().Field("instr_ptr")  // Python 3.13+
			: remoteType().Field("prev_instr");
		return utils::readIntegral<int>(instrPtr);
	}


	auto PyInterpreterFrame::currentLineNumber() const -> int
	{
		auto codeObject = code();

		// Do a lookup into the code object's line number table (co_linetable).
		return codeObject->lineNumberFromPrevInstruction(prevInstruction());
	}
}
