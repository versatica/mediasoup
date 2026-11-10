#include "fuzzer/include/RTC/RTCP/packet/FuzzerRembPacket.hpp"

void FuzzerRtcRtcpRembPacket::Fuzz(const RTC::RTCP::RembPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
	packet->GetBitrate();
	packet->GetNumberOfSsrcs();
	packet->GetSsrcs();
}
