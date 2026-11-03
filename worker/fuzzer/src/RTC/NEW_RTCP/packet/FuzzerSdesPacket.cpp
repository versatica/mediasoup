#include "fuzzer/include/RTC/NEW_RTCP/packet/FuzzerSdesPacket.hpp"

void FuzzerRtcRtcpSdesPacket::Fuzz(const RTC::NEW_RTCP::SdesPacket* packet)
{
	packet->GetNumberOfChunks();

	for (const auto& item : packet->GetItems())
	{
		RTC::NEW_RTCP::SdesPacket::ItemTypeToString(item.type);
	}
}
