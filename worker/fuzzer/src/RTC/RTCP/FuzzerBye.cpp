#include "RTC/RTCP/FuzzerBye.hpp"

void FuzzerRtcRtcpBye::Fuzz(RTC::RTCP::ByePacket* packet)
{
	packet->Serialize(RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();
	packet->AddSsrc(1111);
	packet->SetReason("because!");
	packet->GetReason();
}
