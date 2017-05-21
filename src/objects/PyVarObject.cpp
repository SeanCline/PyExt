#include "PyVarObject.h"

#include "../ExtHelpers.h"

#include <engextcpp.hpp>

#include <string>
using namespace std;

namespace PyExt::Remote {

	PyVarObject::PyVarObject(Offset objectAddress, const string& typeName /*= "PyVarObject"*/)
		: PyObject(objectAddress, typeName)
	{
	}


	auto PyVarObject::size() const -> SSize
	{
		auto sizeField = baseField("ob_size");
		return utils::readIntegral<SSize>(sizeField);
	}

}