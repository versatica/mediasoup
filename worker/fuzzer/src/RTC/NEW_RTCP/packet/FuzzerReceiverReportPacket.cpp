#include "RTC/NEW_RTCP/packet/FuzzerReceiverReportPacket.hpp"

void FuzzerRtcRtcpReceiverReportPacket::Fuzz(const RTC::NEW_RTCP::ReceiverReportPacket* packet)
{
	packet->GetSsrc();
	packet->GetNumberOfReportBlocks();
	packet->GetReportBlocks();
}
