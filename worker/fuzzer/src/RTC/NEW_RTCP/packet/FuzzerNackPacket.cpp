#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerNackPacket.hpp"

void FuzzerRtcRtcpNackPacket::Fuzz(const RTC::NEW_RTCP::NackPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->GetNumberOfItems();
	packet->GetItems();
}
