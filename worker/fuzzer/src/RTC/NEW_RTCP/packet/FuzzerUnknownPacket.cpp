#include "RTC/NEW_RTCP/packet/FuzzerUnknownPacket.hpp"

void FuzzerRtcRtcpUnknownPacket::Fuzz(const RTC::NEW_RTCP::UnknownPacket* packet)
{
	packet->HasUnknownValue();
	packet->GetUnknownValue();
	packet->GetUnknownValueLength();
}
