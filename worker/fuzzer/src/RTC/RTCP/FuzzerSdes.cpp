#include "RTC/RTCP/FuzzerSdes.hpp"

void Fuzzer::RTC::RTCP::Sdes::Fuzz(::RTC::RTCP::SdesPacket* packet)
{
	// packet->Dump();
	packet->Serialize(::RTC::RTCP::Buffer);
	packet->GetCount();
	packet->GetSize();

	// TODO.
	// AddChunk(SdesChunk* chunk);

	auto it = packet->Begin();
	for (; it != packet->End(); ++it)
	{
		auto& chunk = (*it);

		// chunk->Dump();
		chunk->Serialize(::RTC::RTCP::Buffer);
		chunk->GetSize();
		chunk->GetSsrc();
		chunk->SetSsrc(1111);

		// TODO
		// AddItem(SdesItem* item);

		auto it2 = chunk->Begin();
		for (; it2 != chunk->End(); ++it2)
		{
			auto& item = (*it2);

			// item->Dump();
			item->Serialize(::RTC::RTCP::Buffer);
			item->GetSize();

			item->GetType();
			item->GetLength();
			item->GetValue();
		}
	}
}
