#ifndef MS_FUZZER_RTC_NEW_RTCP_TRANSPORT_WIDE_CC_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_TRANSPORT_WIDE_CC_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/TransportWideCcPacket.hpp"

namespace FuzzerRtcRtcpTransportWideCcPacket
{
	void Fuzz(const RTC::NEW_RTCP::TransportWideCcPacket* packet);
} // namespace FuzzerRtcRtcpTransportWideCcPacket

#endif
