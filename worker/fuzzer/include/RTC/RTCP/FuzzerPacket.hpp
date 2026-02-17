#ifndef MS_FUZZER_RTC_RTCP_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_PACKET_HPP

#include "common.hpp"

namespace FuzzerRtcRtcpPacket
{
	void Fuzz(const uint8_t* data, size_t len);
}

#endif
