#define CATCH_CONFIG_MAIN
#include "catch.hpp"

// Disable warnings from the Debugging Tools for Windows headers.
#pragma warning( push )
#pragma warning( disable : 4838 )
#include <engextcpp.hpp>
#pragma warning( pop ) 

// Provide a pretty-printer for the ExtException used by EngExtCpp.
CATCH_TRANSLATE_EXCEPTION(ExtException& ex)
{
	return ex.GetMessageA();
}
