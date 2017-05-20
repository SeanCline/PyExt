#pragma once

#include "RemotePyObject.h"

#include <memory>
#include <string>
#include <vector>
#include <utility>

// Represents a PyDictObject in the debuggee's address space.
class RemotePyDictObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyDictObject(Offset objectAddress);

public: // Members.
	auto pairValues() const -> std::vector<std::pair<std::unique_ptr<RemotePyObject>, std::unique_ptr<RemotePyObject>>>;
	auto repr(bool pretty = true) const -> std::string override;

};