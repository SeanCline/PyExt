#include "PyMemberDef.h"

#include "ExtHelpers.h"

using namespace std;

namespace PyExt::Remote {

	PyMemberDef::~PyMemberDef()
	{
	}


	PyMemberDefAuto::PyMemberDefAuto(Offset objectAddress)
		: RemoteType(objectAddress, "PyMemberDef")
	{
	}


	auto PyMemberDefAuto::name() const -> string
	{
		ExtBuffer<char> buff;
		remoteType().Field("name").Dereference().GetString(&buff);
		return buff.GetBuffer();
	}


	auto PyMemberDefAuto::type() const -> int
	{
		auto type_ = remoteType().Field("type");
		return utils::readIntegral<int>(type_);
	}


	auto PyMemberDefAuto::offset() const -> SSize
	{
		auto offset_ = remoteType().Field("offset");
		return utils::readIntegral<SSize>(offset_);
	}


	PyMemberDefManual::PyMemberDefManual(RemoteType::Offset objectAddress)
		: objectAddress(objectAddress)
	{
	}


	auto PyMemberDefManual::name() const -> string
	{
		ExtBuffer<char> buff;
		ExtRemoteTyped("(char**)@$extin", objectAddress).Dereference().Dereference().GetString(&buff);
		return buff.GetBuffer();
	}


	auto PyMemberDefManual::type() const -> int
	{
		auto addr = objectAddress + utils::getPointerSize();
		auto type_ = ExtRemoteTyped("(int*)@$extin", addr).Dereference();
		return utils::readIntegral<int>(type_);
	}


	auto PyMemberDefManual::offset() const -> RemoteType::SSize
	{
		auto addr = objectAddress + utils::getPointerSize() * 2;
		auto offset_ = ExtRemoteTyped("(Py_ssize_t*)@$extin", addr).Dereference();
		return utils::readIntegral<RemoteType::SSize>(offset_);
	}

}