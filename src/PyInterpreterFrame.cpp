#include "PyInterpreterFrame.h"


#include "PyObject.h"
#include "PyCodeObject.h"
#include "PyDictObject.h"
#include "PyFrameObject.h"
#include "PyFunctionObject.h"

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
			// Mask the _PyStackRef ownership tag bit (bit 0). Python objects are
			// pointer-aligned so bit 0 is always 0 in a valid address; if it is
			// set the reference is a deferred/immortal ref in Python 3.14+.
			auto addr = pyObjAddrs.at(i) & ~(Offset)1;
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
		// In Python 3.14 f_executable and f_funcobj are _PyStackRef (a union with
		// a single 'bits' member). DbgEng's GetUlong64() may return 0 on union-
		// typed fields, so we read .bits directly as a plain integer when present.
		// For PyObject* fields (Python 3.11-3.13) HasField("bits") is false and we
		// fall back to GetPtr(). Bit 0 (deferred-ref ownership tag) is masked either way.
		auto readRef = [](ExtRemoteTyped field) -> Offset {
			if (field.HasField("bits")) {
				auto bitsField = field.Field("bits");
				return utils::readIntegral<Offset>(bitsField) & ~(Offset)1;
			}
			return field.GetPtr() & ~(Offset)1;
		};

		// Try f_executable (Python 3.11+).
		if (remoteType().HasField("f_executable")) {
			auto addr = readRef(remoteType().Field("f_executable"));
			if (addr != 0) {
				auto objPtr = PyObject::make(addr);
				if (auto codePtr = dynamic_cast<PyCodeObject*>(objPtr.get())) {
					objPtr.release();
					return unique_ptr<PyCodeObject>(codePtr);
				}
			}
		}
		// Next, try f_funcobj. Checked unconditionally so it fires even
		// when f_executable is absent from the PDB or holds Py_None.
		if (remoteType().HasField("f_funcobj")) {
			auto funcAddr = readRef(remoteType().Field("f_funcobj"));
			if (funcAddr != 0) {
				auto funcObjPtr = PyObject::make(funcAddr);
				if (auto funcPtr = dynamic_cast<PyFunctionObject*>(funcObjPtr.get()))
					return funcPtr->code();
			}
		}
		// Pre-3.11 fallback.
		return utils::fieldAsPyObject<PyCodeObject>(remoteType(), "f_code");
	}


	auto PyInterpreterFrame::previous() const -> unique_ptr<PyFrame>
	{
		auto previous = remoteType().Field("previous");

		// Skip over any incomplete frames, mirroring CPython's _PyFrame_GetFirstComplete.
		// Python 3.13: FRAME_OWNED_BY_CSTACK = 3 (shim frames).
		// Python 3.14: FRAME_OWNED_BY_INTERPRETER = 3, FRAME_OWNED_BY_CSTACK = 4.
		// In both versions every frame with owner >= 3 is incomplete and should be skipped.
		// see https://github.com/python/cpython/blob/3bd942f106aa36c261a2d90104c027026b2a8fb6/Python/traceback.c#L979-L982
		while (previous.GetPtr() != 0) {
			auto ownerRaw = previous.Field("owner");
			auto owner = utils::readIntegral<int8_t>(ownerRaw);
			if (owner < 3)
				break;
			previous = previous.Field("previous");
		}

		if (previous.GetPtr() == 0)
			return { };

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
		if (codeObject == nullptr)
			return 0;
		return codeObject->lineNumberFromPrevInstruction(prevInstruction());
	}
}
