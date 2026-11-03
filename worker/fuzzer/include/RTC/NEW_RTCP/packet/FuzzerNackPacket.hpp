#ifndef MS_FUZZER_RTC_NEW_RTCP_NACK_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_NACK_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/NackPacket.hpp"

namespace FuzzerRtcRtcpNackPacket
{
	void Fuzz(const RTC::NEW_RTCP::NackPacket* packet);
} // namespace FuzzerRtcRtcpNackPacket

#endif
