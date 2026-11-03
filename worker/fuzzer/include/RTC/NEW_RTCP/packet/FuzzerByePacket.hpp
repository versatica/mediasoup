#ifndef MS_FUZZER_RTC_NEW_RTCP_BYE_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_BYE_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/ByePacket.hpp"

namespace FuzzerRtcRtcpByePacket
{
	void Fuzz(const RTC::NEW_RTCP::ByePacket* packet);
} // namespace FuzzerRtcRtcpByePacket

#endif
