#include "RTC/RTCP/FuzzerXr.hpp"

void Fuzzer::RTC::RTCP::ExtendedReport::Fuzz(::RTC::RTCP::ExtendedReportPacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::Buffer);
	packet->GetCount();
	packet->GetSize();

	packet->GetSsrc();
	packet->SetSsrc(1111);

	auto it = packet->Begin();
	for (; it != packet->End(); ++it)
	{
		auto& report = (*it);

		// report->Dump();
		report->Serialize(::RTC::RTCP::Buffer);
		report->GetSize();
		report->GetType();
	}
}
