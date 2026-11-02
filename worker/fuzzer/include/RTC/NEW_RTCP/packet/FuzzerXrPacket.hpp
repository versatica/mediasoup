#ifndef MS_FUZZER_RTC_NEW_RTCP_XR_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_XR_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/XrPacket.hpp"

namespace FuzzerRtcRtcpXrPacket
{
	void Fuzz(const RTC::NEW_RTCP::XrPacket* packet);
} // namespace FuzzerRtcRtcpXrPacket

#endif
