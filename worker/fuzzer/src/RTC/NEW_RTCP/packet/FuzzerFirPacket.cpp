#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerFirPacket.hpp"

void FuzzerRtcRtcpFirPacket::Fuzz(const RTC::NEW_RTCP::FirPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->GetNumberOfItems();
	packet->GetItems();
}
