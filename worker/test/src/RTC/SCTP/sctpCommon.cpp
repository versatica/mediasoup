#include "RTC/SCTP/sctpCommon.hpp" // in worker/test/include/
#include <cstring>                 // std::memset

namespace RTC
{
	namespace SCTP
	{
		thread_local uint8_t FactoryBuffer[];
		thread_local uint8_t SerializeBuffer[];
		thread_local uint8_t CloneBuffer[];
		thread_local uint8_t DataBuffer[];
		thread_local uint8_t ThrowBuffer[];

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
	} // namespace SCTP
} // namespace RTC
