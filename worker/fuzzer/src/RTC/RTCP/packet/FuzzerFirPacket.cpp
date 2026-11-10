#include "fuzzer/include/RTC/RTCP/packet/FuzzerFirPacket.hpp"

void FuzzerRtcRtcpFirPacket::Fuzz(const RTC::RTCP::FirPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->GetNumberOfItems();
	packet->GetItems();
}
