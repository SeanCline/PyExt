#pragma once

#include <engextcpp.hpp>
#include <type_traits>

/// Reads the correct number of bytes from an ExtRemoteTyped and casts it to the provided integral type.
template <typename Integral>
auto readIntegral(/*const*/ ExtRemoteTyped& remoteData) -> Integral
{
	bool isSigned = std::is_signed_v<Integral>;
	auto size = remoteData.GetTypeSize();
	switch (size) {
	case 1:
		return static_cast<Integral>(isSigned ? remoteData.GetChar() : remoteData.GetUchar());
	case 2:
		return static_cast<Integral>(isSigned ? remoteData.GetShort() : remoteData.GetUshort());
	case 4:
		return static_cast<Integral>(isSigned ? remoteData.GetLong() : remoteData.GetUlong());
	case 8:
		return static_cast<Integral>(isSigned ? remoteData.GetLong64() : remoteData.GetUlong64());
	}

	g_Ext->ThrowInterrupt();
	g_Ext->ThrowRemote(E_INVALIDARG, "Invalid ExtRemoteData size for integral read.");
}