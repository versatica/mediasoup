#ifndef MS_FUZZER_RTC_TEST_SERIALIZABLE_FOO_PACKET_HPP
#define MS_FUZZER_RTC_TEST_SERIALIZABLE_FOO_PACKET_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace FooPacket
		{
			void Fuzz(const uint8_t* data, size_t len);
		}
	} // namespace RTC
} // namespace Fuzzer

#endif
