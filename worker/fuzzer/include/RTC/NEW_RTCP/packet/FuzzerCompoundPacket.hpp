#ifndef MS_FUZZER_RTC_NEW_RTCP_COMPOUND_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_COMPOUND_PACKET_HPP

#include "common.hpp"

namespace FuzzerRtcRtcpCompoundPacket
{
	void Fuzz(const uint8_t* data, size_t len);
} // namespace FuzzerRtcRtcpCompoundPacket

#endif
