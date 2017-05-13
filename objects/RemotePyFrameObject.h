#pragma once

#include "RemotePyVarObject.h"
#include <string>
#include <memory>

class RemotePyDictObject;
class RemotePyCodeObject;
class RemotePyFunctionObject;

// Represents a PyFrameObject in the debuggee's address space.
class RemotePyFrameObject : public RemotePyVarObject
{

public: // Construction/Destruction.
	explicit RemotePyFrameObject(Offset objectAddress);

public: // Members.
	auto locals() const -> std::unique_ptr<RemotePyDictObject>;
	auto globals() const -> std::unique_ptr<RemotePyDictObject>;
	auto builtins() const -> std::unique_ptr<RemotePyDictObject>;
	auto code() const -> std::unique_ptr<RemotePyCodeObject>;
	auto back() const -> std::unique_ptr<RemotePyFrameObject>;
	auto trace() const -> std::unique_ptr<RemotePyFunctionObject>;
	auto lastInstruction() const -> int;
	auto currentLineNumber() const -> int;
	auto repr(bool pretty = true) const -> std::string override;

};