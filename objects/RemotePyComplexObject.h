#pragma once

#include "RemotePyObject.h"
#include <string>
#include <complex>
#include <cstdint>

// Represents a PyComplexObject in the debuggee's address space.
class RemotePyComplexObject : public RemotePyObject
{

public: // Construction/Destruction.
	explicit RemotePyComplexObject(Offset objectAddress);

public: // Members.
	auto complexValue() const -> std::complex<double>;
	auto repr(bool pretty = true) const -> std::string override;

};