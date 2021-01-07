#pragma once

#include <engextcpp.hpp>
#include <type_traits>
#include <vector>
using namespace std::literals::string_literals;

namespace utils {

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
		g_Ext->ThrowRemote(E_INVALIDARG, "Invalid ExtRemoteTyped size for integral read.");
	}


	// Reads an array of elements pointed to by an ExtRemoteData.
	template <typename ElemType>
	auto readArray(/*const*/ ExtRemoteTyped& remoteArray, unsigned long numElements) -> std::vector<ElemType>
	{
		auto remoteData = remoteArray.Dereference();
		const auto remoteSize = remoteData.GetTypeSize();
		if (remoteSize != sizeof(ElemType)) {
			g_Ext->ThrowRemote(E_INVALIDARG, "sizeof(ElemType) does not match size type size for ExtRemoteTyped array read.");
		}

		std::vector<ElemType> buffer(numElements);
		remoteData.ReadBuffer(buffer.data(), numElements*remoteSize);
		return buffer;
	}


	auto escapeDml(const std::string& str)->std::string;
	auto link(const std::string& text, const std::string& cmd, const std::string& alt = ""s) -> std::string;

}