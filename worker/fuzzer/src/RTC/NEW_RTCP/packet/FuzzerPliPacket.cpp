#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerPliPacket.hpp"

void FuzzerRtcRtcpPliPacket::Fuzz(const RTC::NEW_RTCP::PliPacket* packet)
{
	packet->GetSenderSsrc();
	packet->GetMediaSsrc();
	packet->GetMessageType();
	packet->HasUnknownMessageType();
}
