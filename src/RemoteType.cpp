#include "RemoteType.h"

#include <engextcpp.hpp>

#include <string>
#include <memory>
using namespace std;

namespace PyExt::Remote {

	RemoteType::RemoteType(Offset objectAddress, const std::string& symbolName)
		: symbolName_(symbolName),
		  remoteType_(make_shared<ExtRemoteTyped>(symbolName.c_str(), objectAddress, true))
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
		return symbolName_;
	}


	auto RemoteType::remoteType() const -> ExtRemoteTyped&
	{
		return *remoteType_;
	}
}