#include "RTC/ICE/iceCommon.hpp" // in worker/test/include/
#include <cstring>               // std::memset

namespace iceCommon
{
	// NOTE: We don't need `alignas(4)` for STUN Packet parsing. However we do it
	// for consistency with rtpCommon.cpp and sctpCommon.cpp.
	alignas(4) thread_local uint8_t FactoryBuffer[];
	alignas(4) thread_local uint8_t ResponseFactoryBuffer[];
	alignas(4) thread_local uint8_t SerializeBuffer[];
	alignas(4) thread_local uint8_t CloneBuffer[];
	alignas(4) thread_local uint8_t DataBuffer[];
	alignas(4) thread_local uint8_t ThrowBuffer[];

	void ResetBuffers()
	{
		std::memset(FactoryBuffer, 0xAA, sizeof(FactoryBuffer));
		std::memset(ResponseFactoryBuffer, 0xAA, sizeof(ResponseFactoryBuffer));
		std::memset(SerializeBuffer, 0xBB, sizeof(SerializeBuffer));
		std::memset(CloneBuffer, 0xCC, sizeof(CloneBuffer));
		std::memset(DataBuffer, 0xDD, sizeof(DataBuffer));
		std::memset(ThrowBuffer, 0xEE, sizeof(ThrowBuffer));

		for (size_t i = 0; i < 256; ++i)
		{
			DataBuffer[i] = static_cast<uint8_t>(i);
		}
	}
} // namespace iceCommon
