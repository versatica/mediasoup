#ifndef MS_FUZZER_RTC_RTCP_SDES_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_SDES_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/SdesPacket.hpp"

namespace FuzzerRtcRtcpSdesPacket
{
	void Fuzz(const RTC::RTCP::SdesPacket* packet);
} // namespace FuzzerRtcRtcpSdesPacket

#endif
