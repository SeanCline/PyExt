#pragma once

#include "RemotePyObject.h"
#include <string>
#include <memory>

class RemotePyCodeObject;
class RemotePyDictObject;
class RemotePyTupleObject;
class RemotePyDictObject;
class RemotePyListObject;
class RemotePyStringObject;

// Represents a PyFunctionObject in the debuggee's address space.
class RemotePyFunctionObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyFunctionObject(Offset objectAddress);

public: // Members.
	auto code() const -> std::unique_ptr<RemotePyCodeObject>;
	auto globals() const -> std::unique_ptr<RemotePyDictObject>;
	auto defaults() const -> std::unique_ptr<RemotePyTupleObject>;
	auto kwdefaults() const -> std::unique_ptr<RemotePyDictObject>;
	auto closure() const -> std::unique_ptr<RemotePyTupleObject>;
	auto doc() const -> std::unique_ptr<RemotePyObject>;
	auto name() const -> std::unique_ptr<RemotePyStringObject>;
	auto dict() const -> std::unique_ptr<RemotePyDictObject>;
	auto weakreflist() const -> std::unique_ptr<RemotePyListObject>;
	auto module() const -> std::unique_ptr<RemotePyObject>;
	auto annotations() const -> std::unique_ptr<RemotePyDictObject>;
	auto qualname() const -> std::unique_ptr<RemotePyStringObject>;
	auto repr(bool pretty = true) const -> std::string override;

};