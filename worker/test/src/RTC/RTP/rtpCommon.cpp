#include "test/include/RTC/RTP/rtpCommon.hpp" // in worker/test/include/
#include "test/include/testHelpers.hpp"

namespace rtpCommon
{
	// NOTE: Buffers must be 4-byte aligned since RTP packet parsing casts them
	// to structs (e.g. FixedHeader, HeaderExtension) that require 4-byte
	// alignment. Without this, accessing multi-byte fields would be undefined
	// behavior on strict-alignment architectures.
	alignas(4) thread_local uint8_t FactoryBuffer[];
	alignas(4) thread_local uint8_t SerializeBuffer[];
	alignas(4) thread_local uint8_t CloneBuffer[];
	alignas(4) thread_local uint8_t DataBuffer[];
	alignas(4) thread_local uint8_t ThrowBuffer[];
	alignas(4) thread_local uint8_t ReadBuffer[];

	void ResetBuffers()
	{
		helpers::FillBuffer(FactoryBuffer, sizeof(FactoryBuffer), 0xAA);
		helpers::FillBuffer(SerializeBuffer, sizeof(SerializeBuffer), 0xBB);
		helpers::FillBuffer(CloneBuffer, sizeof(CloneBuffer), 0xCC);
		helpers::FillBuffer(DataBuffer, sizeof(DataBuffer), 0x00);
		helpers::FillBuffer(ThrowBuffer, sizeof(ThrowBuffer), 0xEE);
		helpers::FillBuffer(ReadBuffer, sizeof(ReadBuffer), 0xFF);
	}
} // namespace rtpCommon
