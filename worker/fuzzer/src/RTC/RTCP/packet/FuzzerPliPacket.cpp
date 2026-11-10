#include "fuzzer/include/RTC/RTCP/packet/FuzzerPliPacket.hpp"

void FuzzerRtcRtcpPliPacket::Fuzz(const RTC::RTCP::PliPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
}
