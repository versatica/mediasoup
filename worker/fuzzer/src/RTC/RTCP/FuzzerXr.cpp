#include "RTC/RTCP/FuzzerXr.hpp"

void FuzzerRtcRtcpExtendedReport::Fuzz(RTC::RTCP::ExtendedReportPacket* packet)
{
	packet->Serialize(RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();

	packet->GetSsrc();
	packet->SetSsrc(1111);

	for (auto it = packet->Begin(); it != packet->End(); ++it)
	{
		auto& report = (*it);

		report->Serialize(RTC::RTCP::SerializationBuffer);
		report->GetSize();
		report->GetType();
	}
}
