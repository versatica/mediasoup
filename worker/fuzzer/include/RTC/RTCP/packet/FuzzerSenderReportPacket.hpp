#ifndef MS_FUZZER_RTC_RTCP_SENDER_REPORT_PACKET_HPP
#define MS_FUZZER_RTC_RTCP_SENDER_REPORT_PACKET_HPP

#include "common.hpp"
#include "RTC/RTCP/packet/SenderReportPacket.hpp"

namespace FuzzerRtcRtcpSenderReportPacket
{
	void Fuzz(const RTC::RTCP::SenderReportPacket* packet);
} // namespace FuzzerRtcRtcpSenderReportPacket

#endif
