#pragma once

#include "pyextpublic.h"

#include <cstdint>
#include <string>
#include <memory>


class ExtRemoteTyped;

namespace PyExt::Remote {

	/// Represents an instance of a type in the debuggee's address space.
	class PYEXT_PUBLIC RemoteType
	{

	public: // Typedefs.
		using Offset = std::uint64_t;
		using SSize = std::int64_t;

	public: // Construction/Destruction.
		explicit RemoteType(const std::string& objectExpression);
		explicit RemoteType(Offset objectAddress, const std::string& symbolName);
		explicit RemoteType(ExtRemoteTyped remoteType);
		virtual ~RemoteType();

	public: // Copy/Move.
		RemoteType(const RemoteType&);
		RemoteType& operator=(const RemoteType&);
		RemoteType(RemoteType&&);
		RemoteType& operator=(RemoteType&&);

	public: // Methods.
		auto offset() const -> Offset;
		auto symbolName() const -> std::string;
		// necessary for x86 because offsets in remote address space are only 32 Bit
		static auto readOffsetArray(/*const*/ ExtRemoteTyped& remoteArray, std::uint64_t numElements) -> std::vector<Offset>;

	protected: // Helpers for more derived classes.
		/// Access to the instance's memory in the debuggee.
		auto remoteType() const -> ExtRemoteTyped&;

	private: // Data.
#pragma warning (push)
#pragma warning (disable: 4251) //< Hide warnings about exporting private symbols.
		std::string symbolName_;
		std::shared_ptr<ExtRemoteTyped> remoteType_;
#pragma warning (pop)

	};

}