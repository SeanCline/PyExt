#include "PyDictKeysObject.h"

#include "PyDictObject.h"
#include "../ExtHelpers.h"


namespace PyExt::Remote {

	// We use "_dictkeysobject" because "PyDictKeysObject" is not available in Python < 3.11
	PyDictKeysObject::PyDictKeysObject(Offset objectAddress)
		: RemoteType(objectAddress, "_dictkeysobject")
	{
	}


	auto PyDictKeysObject::getEntriesTable() -> ExtRemoteTyped
	{
		auto obj = remoteType();

		// Python <= 3.5 stores a pointer to the entries table in `ma_keys->dk_entries`.
		if (obj.HasField("dk_entries"))
			return obj.Field("dk_entries");

		// Python >= 3.6 uses a "compact" layout where the entries appear after the `ma_keys->dk_indices` table.
		PyObject::SSize size;
		if (obj.HasField("dk_size")) {
			auto sizeField = obj.Field("dk_size");
			size = utils::readIntegral<PyObject::SSize>(sizeField);
		}
		else {
			// Python >= 3.11 stores log2 of size
			auto log2sizeField = obj.Field("dk_log2_size");
			auto log2size = utils::readIntegral<uint8_t>(log2sizeField);
			size = static_cast<PyObject::SSize>(1) << log2size;
		}
		auto pointerSize = utils::getPointerSize();

		int indexSize = 0;
		if (size <= 0xff) {
			indexSize = 1;
		}
		else if (size <= 0xffff) {
			indexSize = 2;
		}
		else if (size <= 0xffffffff) {
			indexSize = 4;
		}
		else {
			indexSize = pointerSize;
		}

		auto indicies = obj.Field("dk_indices"); // 3.6 and 3.7 both have an indicies field.
		ExtRemoteTyped indiciesPtr;
		if (indicies.HasField("as_1")) {
			// Python 3.6 accesses dk_indicies though a union.
			indiciesPtr = indicies.Field("as_1").GetPointerTo();
		}
		else {
			// Python 3.7 accesses it as a char[].
			indiciesPtr = indicies.GetPointerTo();
		}

		auto entriesPtr = indiciesPtr.GetPtr() + (size * indexSize);
		if (obj.HasField("dk_kind")) {  // Python >= 3.11
			auto dk_kind = obj.Field("dk_kind");
			auto kind = utils::readIntegral<int>(dk_kind);
			if (kind != 0)
				return ExtRemoteTyped("PyDictUnicodeEntry", entriesPtr, true);
		}
		return ExtRemoteTyped("PyDictKeyEntry", entriesPtr, true);
	}


	auto PyDictKeysObject::getEntriesTableSize() -> RemoteType::SSize
	{
		auto obj = remoteType();

		// Python 3.5
		if (!obj.HasField("dk_nentries")) {
			auto sizeField = obj.Field("dk_size");
			return utils::readIntegral<RemoteType::SSize>(sizeField);
		}

		// Python >= 3.6
		auto numEntriesField = obj.Field("dk_nentries");
		return utils::readIntegral<RemoteType::SSize>(numEntriesField);
	}

}
