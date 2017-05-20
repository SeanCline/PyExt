#pragma once

#include "RemotePyObject.h"
#include "PyStringValue.h"
#include <string>

// Represents a PyAscii/[Compact]UnicodeObject in the debuggee's address space.
class RemotePyUnicodeObject : public RemotePyObject, public PyStringValue
{

public: // Construction/Destruction.
	explicit RemotePyUnicodeObject(Offset objectAddress);

public: // Enums.
	/// SSTATE.
	enum InterningState { NotInterned, InternedMortal, InternedImortal };
	
	/// PyUnicode_Kind.
	enum class Kind {Wchar, OneByte, TwoByte, FourByte};
	
public: // String state.
	auto interningState() const -> InterningState;
	auto kind() const -> Kind;
	auto isCompact() const -> bool;
	auto isAscii() const -> bool;
	auto isReady() const -> bool;

public: // Members.
	auto stringLength() const -> SSize;
	auto stringValue() const -> std::string override;
	auto repr(bool pretty = true) const -> std::string override;

};