#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerByePacket.hpp"

void FuzzerRtcRtcpByePacket::Fuzz(const RTC::NEW_RTCP::ByePacket* packet)
{
	packet->GetSsrcs();
	packet->HasReason();
	packet->GetReason();
}
