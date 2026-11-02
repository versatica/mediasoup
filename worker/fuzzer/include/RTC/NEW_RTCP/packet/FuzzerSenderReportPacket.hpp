#ifndef MS_FUZZER_RTC_NEW_RTCP_SENDER_REPORT_PACKET_HPP
#define MS_FUZZER_RTC_NEW_RTCP_SENDER_REPORT_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/packet/SenderReportPacket.hpp"

namespace FuzzerRtcRtcpSenderReportPacket
{
	void Fuzz(const RTC::NEW_RTCP::SenderReportPacket* packet);
} // namespace FuzzerRtcRtcpSenderReportPacket

#endif
