#include "test/include/RTC/ICE/iceCommon.hpp" // in worker/test/include/
#include "test/include/testHelpers.hpp"

namespace iceCommon
{
	// NOTE: We don't need `alignas(4)` for STUN packet parsing. However we do it
	// for consistency with rtpCommon.cpp, rtcpCommon.cpp and sctpCommon.cpp.
	alignas(4) thread_local uint8_t FactoryBuffer[];
	alignas(4) thread_local uint8_t ResponseFactoryBuffer[];
	alignas(4) thread_local uint8_t SerializeBuffer[];
	alignas(4) thread_local uint8_t CloneBuffer[];
	alignas(4) thread_local uint8_t DataBuffer[];
	alignas(4) thread_local uint8_t ThrowBuffer[];

	void ResetBuffers()
	{
		helpers::FillBuffer(FactoryBuffer, sizeof(FactoryBuffer), 0xAA);
		helpers::FillBuffer(ResponseFactoryBuffer, sizeof(ResponseFactoryBuffer), 0x55);
		helpers::FillBuffer(SerializeBuffer, sizeof(SerializeBuffer), 0xBB);
		helpers::FillBuffer(CloneBuffer, sizeof(CloneBuffer), 0xCC);
		helpers::FillBuffer(DataBuffer, sizeof(DataBuffer), 0x00);
		helpers::FillBuffer(ThrowBuffer, sizeof(ThrowBuffer), 0xEE);
	}
} // namespace iceCommon
