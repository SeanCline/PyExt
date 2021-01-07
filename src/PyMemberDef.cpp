#include "PyMemberDef.h"

#include "ExtHelpers.h"

using namespace std;

namespace PyExt::Remote {

	PyMemberDef::PyMemberDef(Offset objectAddress)
		: RemoteType(objectAddress, "PyMemberDef")
	{
	}


	PyMemberDef::~PyMemberDef()
	{
	}


	auto PyMemberDef::name() const -> string
	{
		ExtBuffer<char> buff;
		remoteType().Field("name").Dereference().GetString(&buff);
		return buff.GetBuffer();
	}


	auto PyMemberDef::type() const -> int
	{
		auto type_ = remoteType().Field("type");
		return utils::readIntegral<int>(type_);
	}


	auto PyMemberDef::offset() const -> SSize
	{
		auto offset_ = remoteType().Field("offset");
		return utils::readIntegral<SSize>(offset_);
	}

}