#include "RemoteType.h"

#include "ExtHelpers.h"
#include <engextcpp.hpp>

#include <string>
#include <memory>
#include <utility>
using namespace std;

namespace PyExt::Remote {

	RemoteType::RemoteType(const string& objectExpression)
		: remoteType_(make_shared<ExtRemoteTyped>(objectExpression.c_str()))
	{
	}

	RemoteType::RemoteType(Offset objectAddress, const std::string& symbolName)
		: symbolName_(symbolName),
		  remoteType_(make_shared<ExtRemoteTyped>(symbolName.c_str(), objectAddress, true))
	{
	}

	RemoteType::RemoteType(ExtRemoteTyped remoteType)
		: remoteType_(make_shared<ExtRemoteTyped>(std::move(remoteType)))
	{
	}

	RemoteType::~RemoteType() = default;
	RemoteType::RemoteType(const RemoteType&) = default;
	RemoteType& RemoteType::operator=(const RemoteType&) = default;
	RemoteType::RemoteType(RemoteType&&) = default;
	RemoteType& RemoteType::operator=(RemoteType&&) = default;


	auto RemoteType::offset() const -> Offset
	{
		return remoteType().GetPtr();
	}


	auto RemoteType::symbolName() const -> std::string
	{
		return symbolName_.empty() ? remoteType().GetTypeName() : symbolName_;
	}


	auto RemoteType::remoteType() const -> ExtRemoteTyped&
	{
		return *remoteType_;
	}


	auto RemoteType::readOffsetArray(/*const*/ ExtRemoteTyped& remoteArray, std::uint64_t numElements) -> vector<Offset>
	{
		auto ptrSize = utils::getPointerSize();
		switch (ptrSize) {
		case 4: {
			// x86 - 32 Bit Python
			auto ptrs = utils::readArray<std::uint32_t>(remoteArray, numElements);
			vector<Offset> offsets(begin(ptrs), end(ptrs));
			return offsets;
		}
		case 8:
			// x64 - 64 Bit Python
			return utils::readArray<std::uint64_t>(remoteArray, numElements);
		}

		g_Ext->ThrowInterrupt();
		g_Ext->ThrowRemote(E_INVALIDARG, "Unsupported pointer size.");
	}

}