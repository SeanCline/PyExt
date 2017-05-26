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
		: RemoteType(objectAddress, "PyInterpreterState")
	{
	}


	PyInterpreterState::~PyInterpreterState()
	{
	}


	auto PyInterpreterState::makeAutoInterpreterState() -> unique_ptr<PyInterpreterState>
	{
		auto autoInterpreterState = ExtRemoteTyped("autoInterpreterState");
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
