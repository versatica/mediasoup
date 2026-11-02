#ifndef MS_FUZZER_RTC_NEW_RTCP_RECEIVER_REPORT_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_RECEIVER_REPORT_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/ReceiverReportPacket.hpp"

namespace FuzzerRtcRtcpReceiverReportPacket
{
	void Fuzz(const RTC::NEW_RTCP::ReceiverReportPacket* packet);
} // namespace FuzzerRtcRtcpReceiverReportPacket

#endif
