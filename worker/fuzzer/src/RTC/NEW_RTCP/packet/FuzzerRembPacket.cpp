#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerRembPacket.hpp"

void FuzzerRtcRtcpRembPacket::Fuzz(const RTC::NEW_RTCP::RembPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->GetBitrate();
	packet->GetNumberOfSsrcs();
	packet->GetSsrcs();
}
