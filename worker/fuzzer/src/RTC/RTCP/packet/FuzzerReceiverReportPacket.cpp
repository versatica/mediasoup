#include "fuzzer/include/RTC/RTCP/packet/FuzzerReceiverReportPacket.hpp"

void FuzzerRtcRtcpReceiverReportPacket::Fuzz(const RTC::RTCP::ReceiverReportPacket* packet)
{
	packet->GetSsrc();
	packet->GetNumberOfReportBlocks();
	packet->GetReportBlocks();
}
