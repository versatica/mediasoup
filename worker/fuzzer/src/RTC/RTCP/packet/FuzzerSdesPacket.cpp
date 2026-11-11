#include "fuzzer/include/RTC/RTCP/packet/FuzzerSdesPacket.hpp"

void FuzzerRtcRtcpSdesPacket::Fuzz(const RTC::RTCP::SdesPacket* packet)
{
	packet->GetNumberOfChunks();

	for (const auto& item : packet->GetItems())
	{
		RTC::RTCP::SdesPacket::ItemTypeToString(item.type);
	}
}
