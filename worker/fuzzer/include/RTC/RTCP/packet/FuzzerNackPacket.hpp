#ifndef MS_FUZZER_RTC_RTCP_NACK_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_NACK_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/NackPacket.hpp"

namespace FuzzerRtcRtcpNackPacket
{
	void Fuzz(const RTC::RTCP::NackPacket* packet);
} // namespace FuzzerRtcRtcpNackPacket

#endif
