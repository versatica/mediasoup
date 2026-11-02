#ifndef MS_FUZZER_RTC_NEW_RTCP_SDES_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_SDES_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/SdesPacket.hpp"

namespace FuzzerRtcRtcpSdesPacket
{
	void Fuzz(const RTC::NEW_RTCP::SdesPacket* packet);
} // namespace FuzzerRtcRtcpSdesPacket

#endif
