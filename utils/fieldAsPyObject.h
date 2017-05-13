#pragma once

#include "objects/objects.h"

#include <memory>
#include <string>
#include <engextcpp.hpp>

// A helper for derived RemotePy*Objects to construct RemotePy*Objects from their fields.
template <typename T>
auto fieldAsPyObject(ExtRemoteTyped& remoteType, const std::string& fieldName) -> std::unique_ptr<T>
{
	if (!remoteType.HasField(fieldName.c_str()))
		return {};

	auto fieldPtr = remoteType.Field(fieldName.c_str()).GetPtr();
	if (fieldPtr == 0)
		return {};

	auto objPtr = makeRemotePyObject(fieldPtr);
	auto derivedObjPtr = dynamic_cast<T*>(objPtr.get());

	// If the dynamic_cast worked, transfer ownership.
	if (derivedObjPtr != nullptr)
		objPtr.release();

	return std::unique_ptr<T>(derivedObjPtr);
}