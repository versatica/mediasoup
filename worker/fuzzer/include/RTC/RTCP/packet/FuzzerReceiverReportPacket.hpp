#ifndef MS_FUZZER_RTC_RTCP_RECEIVER_REPORT_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_RECEIVER_REPORT_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/ReceiverReportPacket.hpp"

namespace FuzzerRtcRtcpReceiverReportPacket
{
	void Fuzz(const RTC::RTCP::ReceiverReportPacket* packet);
} // namespace FuzzerRtcRtcpReceiverReportPacket

#endif
