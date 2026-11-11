#ifndef MS_FUZZER_RTC_RTCP_PLI_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_PLI_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/PliPacket.hpp"

namespace FuzzerRtcRtcpPliPacket
{
	void Fuzz(const RTC::RTCP::PliPacket* packet);
} // namespace FuzzerRtcRtcpPliPacket

#endif
