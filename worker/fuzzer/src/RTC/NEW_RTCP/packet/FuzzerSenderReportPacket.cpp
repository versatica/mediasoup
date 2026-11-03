#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerSenderReportPacket.hpp"

void FuzzerRtcRtcpSenderReportPacket::Fuzz(const RTC::NEW_RTCP::SenderReportPacket* packet)
{
	packet->GetSsrc();
	packet->GetNtp();
	packet->GetRtpTimestamp();
	packet->GetPacketCount();
	packet->GetOctetCount();
	packet->GetNumberOfReportBlocks();
	packet->GetReportBlocks();
}
