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
	std::complex<double> complexValue() const;
	std::string repr(bool pretty = true) const override;

};