#ifndef MS_FUZZER_RTC_NEW_RTCP_UNKNOWN_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_UNKNOWN_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/UnknownPacket.hpp"

namespace FuzzerRtcRtcpUnknownPacket
{
	void Fuzz(const RTC::NEW_RTCP::UnknownPacket* packet);
} // namespace FuzzerRtcRtcpUnknownPacket

#endif
