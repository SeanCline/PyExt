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

	PyInterpreterFrame::PyInterpreterFrame(const RemoteType& remoteType, std::optional<int> tlbcIndex)
		: RemoteType(remoteType), tlbcIndex_(tlbcIndex)
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
			// GIL build (Python 3.14+) stuffs the ownership tag in the low bit of pointers.
			// On the free-threaded build a _PyStackRef is an index into the per-interpreter
			// stackref table, not a tagged pointer, so this masking yields garbage there.
			// https://huangxt.com/wiki/cpython/tagged-pointer
			auto addr = pyObjAddrs.at(i) & ~(Offset)1; //< Mask off the ownership bit. TODO: Factor this out to a common function.
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
		// On the GIL build (Python 3.14+) f_executable and f_funcobj are _PyStackRef,
		// a union with a single 'bits' member whose low bit is the ownership tag.
		// Because GetUlong64() returns 0 on unions, we read .bits directly.
		// For PyObject* fields (Python 3.11-3.13) HasField("bits") is false and we fall
		// back to GetPtr(). On the free-threaded build _PyStackRef is a 32-bit table
		// index instead — this masking yields garbage there.
		auto readRef = [](ExtRemoteTyped field) -> Offset {
			if (field.HasField("bits")) {
				auto bitsField = field.Field("bits");
				return utils::readIntegral<Offset>(bitsField) & ~(Offset)1;
			}
			return field.GetPtr() & ~(Offset)1; //< Mask off the ownership bit. TODO: Factor this out to a common function.
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
		// Propagate tlbcIndex_ — every frame in the chain belongs to the same
		// thread, so they share the same per-thread bytecode slot.
		while (previous.GetPtr() != 0) {
			PyInterpreterFrame candidate{ RemoteType(previous), tlbcIndex_ };
			if (!candidate.isIncomplete())
				break;
			previous = previous.Field("previous");
		}

		if (previous.GetPtr() == 0)
			return { };

		return make_unique<PyInterpreterFrame>(RemoteType(previous), tlbcIndex_);
	}


	auto PyInterpreterFrame::prevInstruction() const -> RemoteType::Offset
	{
		auto instrPtr = remoteType().HasField("instr_ptr")
			? remoteType().Field("instr_ptr")  // Python 3.13+
			: remoteType().Field("prev_instr");
		return utils::readIntegral<RemoteType::Offset>(instrPtr);
	}


	auto PyInterpreterFrame::isIncomplete() const -> bool
	{
		// Mirrors CPython's _PyFrame_IsIncomplete.
		// Owner constants in 3.14's pycore_interpframe_structs.h are:
		//   THREAD = 0, GENERATOR = 1, FRAME_OBJECT = 2, INTERPRETER = 3, CSTACK = 4.
		// Python 3.13 used CSTACK = 3. Python 3.14 inserts a new INTERPRETER = 3 pushing CSTACK to 4.
		// Comparing owner >= 3 works for both versions' enums.
		auto ownerRaw = remoteType().Field("owner");
		auto owner = utils::readIntegral<int8_t>(ownerRaw);
		if (owner >= 3)
			return true;

		// Frames owned by a generator are always complete.
		constexpr auto FRAME_OWNED_BY_GENERATOR = 1;
		if (owner == FRAME_OWNED_BY_GENERATOR)
			return false;

		
		// If we fail to read the fields we expect to be there, assume its an older version of Python where all frames are complete.
		bool incomplete = false;
		utils::ignoreExtensionError([&] {
			auto codeObject = code();
			if (codeObject == nullptr)
				return;

			auto firstTraceable = codeObject->firstTraceableIndex();
			if (!firstTraceable.has_value() || *firstTraceable <= 0)
				return; //< <3.12, or the function starts traceable immediately.

			auto bytecodeStart = codeObject->bytecodeStartAddress(tlbcIndex_);
			if (!bytecodeStart.has_value())
				return; //< This Python version has no bytecodeStart so the frame must be complete.

			// A frame whose `instr_ptr` is below `_co_firsttraceable` hasn't reached a RESUME yet and is incomplete.
			constexpr auto codeUnitSize = 2; //< _Py_CODEUNIT is a uint16_t (high byte opcode | low byte oparg).
			auto threshold = *bytecodeStart + static_cast<RemoteType::Offset>(*firstTraceable) * codeUnitSize;
			if (prevInstruction() < threshold)
				incomplete = true;
		});
		return incomplete;
	}


	auto PyInterpreterFrame::currentLineNumber() const -> int
	{
		auto codeObject = code();
		if (codeObject == nullptr)
			return 0;
		return codeObject->lineNumberFromPrevInstruction(prevInstruction(), tlbcIndex_);
	}
}
