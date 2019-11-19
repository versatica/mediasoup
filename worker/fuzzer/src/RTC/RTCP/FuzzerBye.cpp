#include "RTC/RTCP/FuzzerBye.hpp"

void Fuzzer::RTC::RTCP::Bye::Fuzz(::RTC::RTCP::ByePacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::Buffer);
	packet->GetCount();
	packet->GetSize();
	packet->AddSsrc(1111);
	packet->SetReason("because!");
	packet->GetReason();
}
