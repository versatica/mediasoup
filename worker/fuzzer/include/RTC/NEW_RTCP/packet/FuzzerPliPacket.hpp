#ifndef MS_FUZZER_RTC_NEW_RTCP_PLI_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_PLI_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/PliPacket.hpp"

namespace FuzzerRtcRtcpPliPacket
{
	void Fuzz(const RTC::NEW_RTCP::PliPacket* packet);
} // namespace FuzzerRtcRtcpPliPacket

#endif
