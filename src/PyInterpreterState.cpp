#include "PyInterpreterState.h"
#include "PyThreadState.h"
#include "PyFrameObject.h"
#include "PyDictObject.h"

#include "fieldAsPyObject.h"

#include <engextcpp.hpp>

#include <cstdint>
#include <memory>
#include <vector>
#include <optional>
using namespace std;

namespace PyExt::Remote {

	PyInterpreterState::PyInterpreterState(Offset objectAddress)
		: RemoteType(objectAddress, pyInterpreterStateTypeName().c_str())
	{
	}


	PyInterpreterState::~PyInterpreterState()
	{
	}

	auto PyInterpreterState::pyInterpreterStateTypeName() -> std::string
	{
		ULONG typeId = 0;
		HRESULT hr = S_OK;
		hr = g_Ext->m_Symbols->GetSymbolTypeId("PyInterpreterState", &typeId, nullptr);
		if (SUCCEEDED(hr))
			return "PyInterpreterState";

		hr = g_Ext->m_Symbols->GetSymbolTypeId("_is", &typeId, nullptr);
		if (SUCCEEDED(hr))
			return "_is";

		throw runtime_error("PyInterpreterState symbol could not be located. Ensure proper symbols are loaded.");
	}


	auto PyInterpreterState::makeAutoInterpreterState() -> unique_ptr<PyInterpreterState>
	{
		bool autoInterpreterStateFound = false;
		ExtRemoteTyped autoInterpreterState;

		// In Python 3.7, the autoInterpreterState has moved into the gilstate. Try it first.
		try {
			autoInterpreterState = ExtRemoteTyped("_PyRuntime.gilstate.autoInterpreterState");
			autoInterpreterStateFound = true;
		}
		catch (ExtException&) {}
		
		if (!autoInterpreterStateFound) {
			// Fall back on the pre-3.7 global autoInterpreterState.
			try {
				autoInterpreterState = ExtRemoteTyped("autoInterpreterState");
				autoInterpreterStateFound = true;
			}
			catch (ExtException&) {}
		}

		if (!autoInterpreterStateFound) {
			// The fallback failed. Report the error.
			throw runtime_error("Could not find autoInterpreterState.");
		}

		return make_unique<PyInterpreterState>(autoInterpreterState.GetPtr());
	}


	auto PyInterpreterState::allInterpreterStates() -> std::vector<PyInterpreterState>
	{
		vector<PyInterpreterState> states;
		for (auto state = makeAutoInterpreterState(); state != nullptr; state = state->next()) {
			states.push_back(*state);
		}
		return states;
	}


	auto PyInterpreterState::findThreadStateBySystemThreadId(uint64_t systemThreadId) -> optional<PyThreadState>
	{
		for (auto istate = makeAutoInterpreterState(); istate != nullptr; istate = istate->next()) {
			for (auto tstate = istate->tstate_head(); tstate != nullptr; tstate = tstate->next()) {
				if (tstate->thread_id() == systemThreadId) {
					return move(*tstate);
				}
			}
		}

		return { };
	}


	auto PyInterpreterState::next() const -> std::unique_ptr<PyInterpreterState>
	{
		auto nextPtr = remoteType().Field("next").GetPtr();
		if (nextPtr == 0)
			return { };

		return make_unique<PyInterpreterState>(nextPtr);
	}


	auto PyInterpreterState::tstate_head() const -> unique_ptr<PyThreadState>
	{
		return make_unique<PyThreadState>(remoteType().Field("tstate_head").GetPtr());
	}


	auto PyInterpreterState::modules() const -> std::unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "modules");
	}


	auto PyInterpreterState::sysdict() const -> std::unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "sysdict");
	}


	auto PyInterpreterState::builtins() const -> std::unique_ptr<PyDictObject>
	{
		return utils::fieldAsPyObject<PyDictObject>(remoteType(), "builtins");
	}


	auto PyInterpreterState::allThreadStates() const -> std::vector<PyThreadState>
	{
		vector<PyThreadState> states;
		for (auto state = tstate_head(); state != nullptr; state = state->next()) {
			states.push_back(*state);
		}
		return states;
	}

}
