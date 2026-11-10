#include "fuzzer/include/RTC/RTCP/packet/FuzzerNackPacket.hpp"

void FuzzerRtcRtcpNackPacket::Fuzz(const RTC::RTCP::NackPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->GetNumberOfItems();
	packet->GetItems();
}
