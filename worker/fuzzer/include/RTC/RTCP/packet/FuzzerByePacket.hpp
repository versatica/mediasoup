#ifndef MS_FUZZER_RTC_RTCP_BYE_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_BYE_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/ByePacket.hpp"

namespace FuzzerRtcRtcpByePacket
{
	void Fuzz(const RTC::RTCP::ByePacket* packet);
} // namespace FuzzerRtcRtcpByePacket

#endif
