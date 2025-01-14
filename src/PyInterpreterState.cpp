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

	PyInterpreterState::PyInterpreterState(const RemoteType& remoteType)
		: RemoteType(remoteType)
	{
	}
	

	PyInterpreterState::~PyInterpreterState()
	{
	}


	auto PyInterpreterState::makeAutoInterpreterState() -> unique_ptr<PyInterpreterState>
	{
		string errorMessage; // Build a string of everything that went wrong.

		// Capture any errors that occured. Only print them when none of the fallbacks were successful.
		ExtCaptureOutputA errorOutput;
		errorOutput.Start();

		// See if a autoInterpreterState offset has been manually provided.
		if (!autoInterpreterStateExpressionOverride.empty()) {
			try {
				return make_unique<PyInterpreterState>(RemoteType(autoInterpreterStateExpressionOverride));
			} catch (ExtException& ex) {
				errorMessage += ex.GetMessage() + "\n"s;
			};
		}
		
		// In Python 3.7, the autoInterpreterState has moved into the gilstate. Try it first.
		try {
			return make_unique<PyInterpreterState>(RemoteType("_PyRuntime.gilstate.autoInterpreterState"s));
		} catch (ExtException& ex) {
			errorMessage += ex.GetMessage() + "\n"s;
		};

		// Fall back on the pre-3.7 global autoInterpreterState.
		try {
			return make_unique<PyInterpreterState>(RemoteType("autoInterpreterState"s));
		} catch (ExtException& ex) {
			errorMessage += ex.GetMessage() + "\n"s;
		};

		// All fallbacks failed. Report the error.
		throw runtime_error("Could not find autoInterpreterState.\n"s + errorMessage + "\n"s + errorOutput.GetTextNonNull());
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

	string PyInterpreterState::autoInterpreterStateExpressionOverride;
	void PyInterpreterState::setAutoInterpreterStateExpression(const string& expression)
	{
		// TODO: Consider validating the expression here rather than only in makeAutoInterpreterState.
		autoInterpreterStateExpressionOverride = expression;
	}


	auto PyInterpreterState::next() const -> std::unique_ptr<PyInterpreterState>
	{
		auto next = remoteType().Field("next");
		if (next.GetPtr() == 0)
			return { };

		return make_unique<PyInterpreterState>(RemoteType(next));
	}


	auto PyInterpreterState::tstate_head() const -> unique_ptr<PyThreadState>
	{
		if (remoteType().HasField("tstate_head")) {
			// Old, pre-3.11 location of tstate_head.
			return make_unique<PyThreadState>(RemoteType(remoteType().Field("tstate_head")));
		} else {
			// New, 3.11+ location of the head thread.
			return make_unique<PyThreadState>(RemoteType(remoteType().Field("threads").Field("head")));
		}
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
