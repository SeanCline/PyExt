#include "extension.h"

#include "PyInterpreterState.h"
using namespace PyExt::Remote;

#include "ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
#include <stdexcept>
using namespace std;


namespace PyExt {

	EXT_COMMAND(pysetautointerpreterstate, "Manually provide the address of the PyInterpreterState struct. Use only when PyExt fails to detect it automatically.", "{;x,o;PyInterpreterState Expression}")
	{
		ensureSymbolsLoaded();

		string objExpression = (m_NumUnnamedArgs < 1) ? "" : GetUnnamedArgStr(0);

		// No autointerpreterstate expression provided. Revert to the default.
		if (objExpression.empty()) {
			Out("Using default AutoInterpreterState.\n");
			PyInterpreterState::setAutoInterpreterStateExpression("");
			return;
		}

		// Validate the expression before passing it onto PyInterpreterState.
		try {	
			ExtRemoteTyped remoteObj(objExpression.c_str());
		} catch (ExtException&) {
			Out("PySetAutoInterpreterState failed to evaluate expression: %s\n", objExpression.c_str());
			throw;
		}

		// If we got here then the provided expression is at least valid.
		PyInterpreterState::setAutoInterpreterStateExpression(objExpression);
		Out("AutoInterpreterState set to: %s\n", objExpression.c_str());

		Out("\n");
	}

}
