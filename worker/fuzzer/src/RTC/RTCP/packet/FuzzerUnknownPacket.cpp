#include "fuzzer/include/RTC/RTCP/packet/FuzzerUnknownPacket.hpp"

void FuzzerRtcRtcpUnknownPacket::Fuzz(const RTC::RTCP::UnknownPacket* packet)
{
	packet->HasUnknownValue();
	packet->GetUnknownValue();
	packet->GetUnknownValueLength();
}
