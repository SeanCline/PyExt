#include "PyInterpreterState.h"
#include "PyThreadState.h"
#include "PyFrameObject.h"
#include "PyDictObject.h"

#include "fieldAsPyObject.h"
#include "ExtHelpers.h"

#include <engextcpp.hpp>

#include <cstdint>
#include <generator>
#include <initializer_list>
#include <memory>
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


	auto PyInterpreterState::makeHeadInterpreterState() -> unique_ptr<PyInterpreterState>
	{
		// A manually provided interpreter state is taken to be the place to start.
		if (autoInterpreterStateExpressionOverride.empty()) {
			// Python 3.7+ keeps the list in the runtime state. Before that it was a static variable.
			for (auto expression : { "_PyRuntime.interpreters.head", "interp_head" }) {
				unique_ptr<PyInterpreterState> head;
				utils::ignoreExtensionError([&] {
					auto remoteHead = RemoteType(string(expression));
					if (remoteHead.offset() != 0)
						head = make_unique<PyInterpreterState>(remoteHead);
				});
				if (head != nullptr)
					return head;
			}
		}

		// The main interpreter is the end of the list, which is better than nothing.
		return makeAutoInterpreterState();
	}


	auto PyInterpreterState::allInterpreterStates() -> std::generator<PyInterpreterState>
	{
		for (auto state = makeHeadInterpreterState(); state != nullptr; state = state->next()) {
			co_yield *state;
		}
	}


	auto PyInterpreterState::findThreadStateBySystemThreadId(uint64_t systemThreadId) -> optional<PyThreadState>
	{
		// Starting from the head matters when a thread has called into a sub-interpreter. It then has a
		// thread state in both interpreters, and the newer interpreter's is the one that's running.
		for (auto istate = makeHeadInterpreterState(); istate != nullptr; istate = istate->next()) {
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
		auto head = remoteType().HasField("tstate_head")
			? remoteType().Field("tstate_head")              // Old, pre-3.11 location of tstate_head.
			: remoteType().Field("threads").Field("head");   // New, 3.11+ location of the head thread.

		// An interpreter that is being created or torn down has no threads.
		if (head.GetPtr() == 0)
			return { };
		return make_unique<PyThreadState>(RemoteType(head));
	}


	auto PyInterpreterState::allThreadStates() const -> std::generator<PyThreadState>
	{
		for (auto state = tstate_head(); state != nullptr; state = state->next()) {
			co_yield *state;
		}
	}

}
