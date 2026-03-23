#include "RTC/RTP/rtpCommon.hpp" // in worker/test/include/
#include <cstring>               // std::memset

namespace rtpCommon
{
	// NOTE: Buffers must be 4-byte aligned since RTP Packet parsing casts them
	// to structs (e.g. FixedHeader, HeaderExtension) that require 4-byte
	// alignment. Without this, accessing multi-byte fields would be undefined
	// behavior on strict-alignment architectures.
	alignas(4) thread_local uint8_t FactoryBuffer[];
	alignas(4) thread_local uint8_t SerializeBuffer[];
	alignas(4) thread_local uint8_t CloneBuffer[];
	alignas(4) thread_local uint8_t DataBuffer[];
	alignas(4) thread_local uint8_t ThrowBuffer[];

	void ResetBuffers()
	{
		std::memset(FactoryBuffer, 0xAA, sizeof(FactoryBuffer));
		std::memset(SerializeBuffer, 0xBB, sizeof(SerializeBuffer));
		std::memset(CloneBuffer, 0xCC, sizeof(CloneBuffer));
		std::memset(DataBuffer, 0xDD, sizeof(DataBuffer));
		std::memset(ThrowBuffer, 0xEE, sizeof(ThrowBuffer));

		for (size_t i = 0; i < 256; ++i)
		{
			DataBuffer[i] = static_cast<uint8_t>(i);
		}
	}
} // namespace rtpCommon
