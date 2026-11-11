#include "fuzzer/include/RTC/RTCP/packet/FuzzerByePacket.hpp"

void FuzzerRtcRtcpByePacket::Fuzz(const RTC::RTCP::ByePacket* packet)
{
	packet->GetSsrcs();
	packet->HasReason();
	packet->GetReason();
}
