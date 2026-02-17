#include "RTC/RTCP/FuzzerSdes.hpp"

void FuzzerRtcRtcpSdes::Fuzz(RTC::RTCP::SdesPacket* packet)
{
	packet->Serialize(RTC::RTCP::SerializationBuffer);
	packet->GetCount();
	packet->GetSize();

	// TODO.
	// AddChunk(SdesChunk* chunk);

	for (auto it = packet->Begin(); it != packet->End(); ++it)
	{
		auto& chunk = (*it);

		chunk->Serialize(RTC::RTCP::SerializationBuffer);
		chunk->GetSize();
		chunk->GetSsrc();
		chunk->SetSsrc(1111);

		// TODO
		// AddItem(SdesItem* item);

		for (auto it2 = chunk->Begin(); it2 != chunk->End(); ++it2)
		{
			auto& item = (*it2);

			item->Serialize(RTC::RTCP::SerializationBuffer);
			item->GetSize();
			item->GetType();
			item->GetLength();
			item->GetValue();
		}
	}
}
