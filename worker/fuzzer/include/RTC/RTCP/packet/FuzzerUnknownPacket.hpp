#ifndef MS_FUZZER_RTC_RTCP_UNKNOWN_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_UNKNOWN_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/UnknownPacket.hpp"

namespace FuzzerRtcRtcpUnknownPacket
{
	void Fuzz(const RTC::RTCP::UnknownPacket* packet);
} // namespace FuzzerRtcRtcpUnknownPacket

#endif
