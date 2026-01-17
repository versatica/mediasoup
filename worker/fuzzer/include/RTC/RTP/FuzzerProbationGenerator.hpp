#ifndef MS_FUZZER_RTC_RTP_PROBATION_PACKET_HPP
#define MS_FUZZER_RTC_RTP_PROBATION_PACKET_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace RTP
		{
			namespace ProbationGenerator
			{
				void Fuzz(const uint8_t* data, size_t len);
			}
		} // namespace RTP
	} // namespace RTC
} // namespace Fuzzer

#endif
