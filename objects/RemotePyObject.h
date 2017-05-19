#pragma once

#include <engextcpp.hpp>

#include <cstdint>
#include <string>
#include <memory>

class RemotePyTypeObject;

// Represents a PyObject in the debuggee's address space. Base class for all types of PyObject.
class RemotePyObject
{

public: // Typedefs.
	using Offset = std::uint64_t;
	using SSize = std::int64_t;

public: // Construction/Destruction.
	explicit RemotePyObject(Offset objectAddress, const std::string& symbolName = "PyObject");
	virtual ~RemotePyObject();

public: // Members.
	auto offset() const -> Offset;
	auto symbolName() const -> std::string;
	auto refCount() const -> SSize;
	auto type() const -> RemotePyTypeObject;
	virtual auto repr(bool pretty = true) const -> std::string;

protected: // Helpers for more derived classes.
	// Access to the PyObject's memory in the debuggee.
	auto remoteObj() const -> ExtRemoteTyped&;

	// Returns a field by name in the `ob_base` member.
	auto baseField(const std::string& fieldName) const -> ExtRemoteTyped;

private: // Data.
	std::shared_ptr<ExtRemoteTyped> remoteObj_;
	std::string symbolName_;

};
