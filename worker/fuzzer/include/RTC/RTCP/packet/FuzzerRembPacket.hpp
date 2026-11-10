#ifndef MS_FUZZER_RTC_RTCP_REMB_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_REMB_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/RembPacket.hpp"

namespace FuzzerRtcRtcpRembPacket
{
	void Fuzz(const RTC::RTCP::RembPacket* packet);
} // namespace FuzzerRtcRtcpRembPacket

#endif
