#pragma once

#include <engextcpp.hpp>
#include <type_traits>
#include <vector>
#include <string>
#include <limits>
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
			return isSigned ? static_cast<Integral>(remoteData.GetChar()) : static_cast<Integral>(remoteData.GetUchar());
		case 2:
			return isSigned ? static_cast<Integral>(remoteData.GetShort()) : static_cast<Integral>(remoteData.GetUshort());
		case 4:
			return isSigned ? static_cast<Integral>(remoteData.GetLong()) : static_cast<Integral>(remoteData.GetUlong());
		case 8:
			return isSigned ? static_cast<Integral>(remoteData.GetLong64()) : static_cast<Integral>(remoteData.GetUlong64());
		}

		g_Ext->ThrowInterrupt();
		g_Ext->ThrowRemote(E_INVALIDARG, "Invalid ExtRemoteTyped size for integral read.");
	}


	// Reads an array of elements pointed to by an ExtRemoteData.
	template <typename ElemType>
	auto readArray(/*const*/ ExtRemoteTyped& remoteArray, std::uint64_t numElements) -> std::vector<ElemType>
	{
		if (numElements == 0)
			return { };

		auto remoteData = remoteArray.Dereference();
		const auto remoteSize = remoteData.GetTypeSize();
		if (remoteSize != sizeof(ElemType)) {
			g_Ext->ThrowRemote(E_INVALIDARG, "sizeof(ElemType) does not match size type size for ExtRemoteTyped array read.");
		}

		const auto arrayExtent = numElements * remoteSize;
		// ExtRemoteData only supports arrays with 32bit extents, so we have to narrow it.
		if (numElements > std::numeric_limits<std::uint32_t>::max()) {
			g_Ext->ThrowRemote(E_BOUNDS, "Could not read array. Size too large.");
		}

		std::vector<ElemType> buffer(numElements);
		remoteData.ReadBuffer(buffer.data(), static_cast<std::uint32_t>(arrayExtent));
		return buffer;
	}


	template<typename T>
	auto ignoreExtensionError(T&& function) -> void
	{
		ExtCaptureOutputA ignoreOut;
		ignoreOut.Start();
		try {
			function();
		} catch (ExtException&) { }
	}


	auto getPointerSize() -> int;
	auto escapeDml(const std::string& str) -> std::string;
	auto link(const std::string& text, const std::string& cmd, const std::string& alt = ""s) -> std::string;
	auto getFullSymbolName(const std::string& symbolName) -> std::string;

}