#include "fuzzer/include/RTC/RTCP/packet/FuzzerSenderReportPacket.hpp"

void FuzzerRtcRtcpSenderReportPacket::Fuzz(const RTC::RTCP::SenderReportPacket* packet)
{
	packet->GetSsrc();
	packet->GetNtp();
	packet->GetRtpTimestamp();
	packet->GetPacketCount();
	packet->GetOctetCount();
	packet->GetNumberOfReportBlocks();
	packet->GetReportBlocks();
}
