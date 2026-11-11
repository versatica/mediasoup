#ifndef MS_FUZZER_RTC_RTCP_XR_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_XR_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/XrPacket.hpp"

namespace FuzzerRtcRtcpXrPacket
{
	void Fuzz(const RTC::RTCP::XrPacket* packet);
} // namespace FuzzerRtcRtcpXrPacket

#endif
