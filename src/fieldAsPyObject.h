#pragma once

#include "PyObject.h"

#include <memory>
#include <string>
#include <engextcpp.hpp>

namespace utils {

	// A helper for derived Py*Objects to construct Py*Objects from their fields.
	template <typename T>
	auto fieldAsPyObject(ExtRemoteTyped& remoteType, const std::string& fieldName) -> std::unique_ptr<T>
	{
		if (!remoteType.HasField(fieldName.c_str()))
			return {};

		auto fieldPtr = remoteType.Field(fieldName.c_str()).GetPtr();
		if (fieldPtr == 0)
			return {};

		auto objPtr = PyExt::Remote::PyObject::make(fieldPtr);
		auto derivedObjPtr = dynamic_cast<T*>(objPtr.get());

		// If the dynamic_cast worked, transfer ownership.
		if (derivedObjPtr != nullptr)
			objPtr.release();

		return std::unique_ptr<T>(derivedObjPtr);
	}

}