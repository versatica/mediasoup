#ifndef MS_FUZZER_RTC_RTCP_TRANSPORT_WIDE_CC_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_TRANSPORT_WIDE_CC_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/TransportWideCcPacket.hpp"

namespace FuzzerRtcRtcpTransportWideCcPacket
{
	void Fuzz(const RTC::RTCP::TransportWideCcPacket* packet);
} // namespace FuzzerRtcRtcpTransportWideCcPacket

#endif
