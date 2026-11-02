#include "RTC/NEW_RTCP/packet/FuzzerSenderReportPacket.hpp"

void FuzzerRtcRtcpSenderReportPacket::Fuzz(const RTC::NEW_RTCP::SenderReportPacket* packet)
{
	packet->GetSsrc();
	packet->GetNtpSec();
	packet->GetNtpFrac();
	packet->GetRtpTimestamp();
	packet->GetPacketCount();
	packet->GetOctetCount();
	packet->GetNumberOfReportBlocks();
	packet->GetReportBlocks();
}
