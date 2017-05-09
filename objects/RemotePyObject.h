#pragma once

#include <engextcpp.hpp>

#include <cstdint>
#include <string>


// Represents a PyObject in the debuggee's address space. Base class for all types of PyObject.
class RemotePyObject
{

public: // Typdefs.
	using Offset = std::uint64_t;
	using SSize = std::int64_t;

public: // Construction/Destruction.
	RemotePyObject(Offset objectAddress, const std::string& typeName = "PyObject");
	virtual ~RemotePyObject();

public: // Members.
	long long getRefCount() const;
	std::string getTypeName() const;
	virtual std::string repr(bool pretty = true) const;

protected: // Helpers for more derived classes.
	ExtRemoteTyped remoteObj() const;

private: // Data.
	mutable ExtRemoteTyped remoteObj_;

};
