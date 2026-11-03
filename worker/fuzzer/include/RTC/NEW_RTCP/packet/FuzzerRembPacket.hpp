#ifndef MS_FUZZER_RTC_NEW_RTCP_REMB_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_REMB_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/RembPacket.hpp"

namespace FuzzerRtcRtcpRembPacket
{
	void Fuzz(const RTC::NEW_RTCP::RembPacket* packet);
} // namespace FuzzerRtcRtcpRembPacket

#endif
