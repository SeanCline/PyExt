#pragma once

#include "RemoteType.h"

namespace PyExt::Remote {

	/// Represents a PyDictKeysObject in the debuggee's address space.
	class PYEXT_PUBLIC PyDictKeysObject : private RemoteType
	{

	public: // Construction/Destruction.
		explicit PyDictKeysObject(Offset objectAddress);

	public: // Members.
		auto getEntriesTable() -> ExtRemoteTyped;
		auto getEntriesTableSize() -> RemoteType::SSize;
		using RemoteType::remoteType;

	};
}
