#include "common.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include <catch2/catch.hpp>
#include <cstring> // std::memcmp()
#include <string>

// TODO: Remove
#define MS_CLASS "RTC::RTCP::TestSdes"
#include "Logger.hpp"

using namespace RTC::RTCP;

namespace TestSdes
{
	// RTCP Sdes Packet.

	// clang-format off
	uint8_t buffer1[] =
	{
		0x81, 0xca, 0x00, 0x06, // Type: 202 (SDES), Count: 1, Length: 6
		0x9f, 0x65, 0xe7, 0x42, // SSRC: 0x9f65e742
		// Chunk 1
		0x01, 0x10, 0x74, 0x37, // Item Type: 1 (CNAME), Length: 16, Value: t7mkYnCm46OcINy/
		0x6d, 0x6b, 0x59, 0x6e,
		0x43, 0x6d, 0x34, 0x36,
		0x4f, 0x63, 0x49, 0x4e,
		0x79, 0x2f, 0x00, 0x00  // 2 null octets
	};
	// clang-format on

	uint8_t* chunkBuffer1 = buffer1 + Packet::CommonHeaderSize;

	// First chunk (chunk 1).
	uint32_t ssrc1{ 0x9f65e742 };
	// First item (item 1).
	SdesItem::Type item1Type{ SdesItem::Type::CNAME };
	std::string item1Value{ "t7mkYnCm46OcINy/" };
	size_t item1Length{ 16u };

	// clang-format off
	uint8_t buffer2[] =
	{
		0x82, 0xca, 0x00, 0x0c, // Type: 202 (SDES), Count: 2, Length: 12
		// Chunk 2
		0x00, 0x00, 0x04, 0xd2, // SSRC: 1234
		0x01, 0x06, 0x71, 0x77, // Item Type: 1 (CNAME), Length: 6, Text: "qwerty"
		0x65, 0x72, 0x74, 0x79,
		0x06, 0x06, 0x69, 0xc3, // Item Type: 6 (TOOL), Length: 6, Text: "iñaki"
		0xb1, 0x61, 0x6b, 0x69,
		0x00, 0x00, 0x00, 0x00, // 4 null octets
		// Chunk 3
		0x00, 0x00, 0x16, 0x2e, // SSRC: 5678
		0x05, 0x11, 0x73, 0x6f, // Item Type: 5 (LOC), Length: 17, Text: "somewhere œæ€"
		0x6d, 0x65, 0x77, 0x68,
		0x65, 0x72, 0x65, 0x20,
		0xc5, 0x93, 0xc3, 0xa6,
		0xe2, 0x82, 0xac, 0x00  // 1 null octet
	};
	// clang-format on

	uint8_t* chunkBuffer2 = buffer2 + Packet::CommonHeaderSize;

	// First chunk (chunk 2).
	uint32_t ssrc2{ 1234 };
	// First item (item 2).
	SdesItem::Type item2Type{ SdesItem::Type::CNAME };
	std::string item2Value{ "qwerty" };
	size_t item2Length{ 6u };
	// First item (item 3).
	SdesItem::Type item3Type{ SdesItem::Type::TOOL };
	std::string item3Value{ "iñaki" };
	size_t item3Length{ 6u };

	// Second chunk (chunk 3).
	uint32_t ssrc3{ 5678 };
	// First item (item 4).
	SdesItem::Type item4Type{ SdesItem::Type::LOC };
	std::string item4Value{ "somewhere œæ€" };
	size_t item4Length{ 17u };
} // namespace TestSdes

using namespace TestSdes;

SCENARIO("RTCP SDES parsing", "[parser][rtcp][sdes]")
{
	SECTION("parse packet 1")
	{
		SdesPacket* packet = SdesPacket::Parse(buffer1, sizeof(buffer1));
		auto* header       = reinterpret_cast<RTC::RTCP::Packet::CommonHeader*>(buffer1);
		size_t chunkIdx{ 0u };
		size_t itemIdx{ 0u };

		REQUIRE(ntohs(header->length) == 6);
		REQUIRE(packet->GetSize() == 28);
		REQUIRE(packet->GetCount() == 1);

		// TODO: We never exit this loop!!!
		// for (auto* chunk = *(packet->Begin()); chunk != *(packet->End()); ++chunk, ++chunkIdx)
		// {
		// 	MS_DUMP("---- parse packet 1 | chunkIdx:%zu, chunk address:%llu", chunkIdx, chunk);

		// 	switch (chunkIdx)
		// 	{
		// 		/* First chunk (chunk 1). */
		// 		case 0:
		// 		{
		// 			// Chunk size must be 24 bytes (including 4 null octets).
		// 			REQUIRE(chunk->GetSize() == 24);
		// 			REQUIRE(chunk->GetSsrc() == ssrc1);

		// 			// TODO: We never exit this loop!!!
		// 			for (auto* item = *(chunk->Begin()); item != *(chunk->End()); ++item, ++itemIdx)
		// 			{
		// 				MS_DUMP("---- parse packet 1 | itemIdx:%zu, item address:%llu", itemIdx, item);

		// 				switch (itemIdx)
		// 				{
		// 					/* First item (item 1). */
		// 					case 0:
		// 					{
		// 						REQUIRE(item->GetType() == item1Type);
		// 						REQUIRE(item->GetLength() == item1Length);
		// 						REQUIRE(std::string(item->GetValue(), item1Length) == item1Value);

		// 						break;
		// 					}
		// 				}
		// 			}

		// 			// There is 1 item.
		// 			REQUIRE(itemIdx == 0);

		// 			SECTION("serialize SdesChunk instance")
		// 			{
		// 				// NOTE: Length of first chunk (including null octets) is 24.
		// 				uint8_t serialized[24] = { 0 };

		// 				chunk->Serialize(serialized);

		// 				SECTION("compare serialized SdesChunk with original buffer")
		// 				{
		// 					REQUIRE(std::memcmp(chunkBuffer1, serialized, 24) == 0);
		// 				}
		// 			}

		// 			break;
		// 		}
		// 	}
		// }

		// // There is 1 chunk.
		// REQUIRE(chunkIdx == 0);

		delete packet;
	}

	SECTION("parse packet 2")
	{
		SdesPacket* packet = SdesPacket::Parse(buffer2, sizeof(buffer2));
		auto* header       = reinterpret_cast<RTC::RTCP::Packet::CommonHeader*>(buffer2);
		size_t chunkIdx{ 0u };
		size_t itemIdx{ 0u };

		REQUIRE(ntohs(header->length) == 12);
		REQUIRE(packet->GetSize() == 52);
		REQUIRE(packet->GetCount() == 2);

		// for (auto* chunk = *(packet->Begin()); chunk != *(packet->End()); ++chunk, ++chunkIdx)
		// {
		// 	switch (chunkIdx)
		// 	{
		// 		/* First chunk (chunk 2). */
		// 		case 0:
		// 		{
		// 			// Chunk size must be 24 bytes (including 4 null octets).
		// 			REQUIRE(chunk->GetSize() == 24);
		// 			REQUIRE(chunk->GetSsrc() == ssrc2);

		// 			for (auto* item = *(chunk->Begin()); item != *(chunk->End()); ++item, ++itemIdx)
		// 			{
		// 				switch (itemIdx)
		// 				{
		// 					/* First item (item 2). */
		// 					case 0:
		// 					{
		// 						REQUIRE(item->GetType() == item2Type);
		// 						REQUIRE(item->GetLength() == item2Length);
		// 						REQUIRE(std::string(item->GetValue(), item2Length) == item2Value);

		// 						break;
		// 					}

		// 					/* Second item (item 3). */
		// 					case 1:
		// 					{
		// 						REQUIRE(item->GetType() == item3Type);
		// 						REQUIRE(item->GetLength() == item3Length);
		// 						REQUIRE(std::string(item->GetValue(), item3Length) == item3Value);

		// 						break;
		// 					}
		// 				}
		// 			}

		// 			// There are 2 items.
		// 			REQUIRE(itemIdx == 1);

		// 			SECTION("serialize SdesChunk instance")
		// 			{
		// 				// NOTE: Length of first chunk (including null octets) is 24.
		// 				uint8_t serialized[24] = { 0 };

		// 				chunk->Serialize(serialized);

		// 				SECTION("compare serialized SdesChunk with original buffer")
		// 				{
		// 					REQUIRE(std::memcmp(chunkBuffer2, serialized, 24) == 0);
		// 				}
		// 			}

		// 			break;
		// 		}

		// 		/* Second chunk (chunk 3). */
		// 		case 1:
		// 		{
		// 			// Chunk size must be 24 bytes (including 1 null octet).
		// 			REQUIRE(chunk->GetSize() == 24);
		// 			REQUIRE(chunk->GetSsrc() == ssrc3);

		// 			for (auto* item = *(chunk->Begin()); item != *(chunk->End()); ++item, ++itemIdx)
		// 			{
		// 				switch (itemIdx)
		// 				{
		// 					/* First item (item 4). */
		// 					case 0:
		// 					{
		// 						REQUIRE(item->GetType() == item4Type);
		// 						REQUIRE(item->GetLength() == item4Length);
		// 						REQUIRE(std::string(item->GetValue(), item4Length) == item4Value);

		// 						break;
		// 					}
		// 				}
		// 			}

		// 			// There is 1 item.
		// 			REQUIRE(itemIdx == 0);

		// 			SECTION("serialize SdesChunk instance")
		// 			{
		// 				// NOTE: Length of second chunk (including null octets) is 24.
		// 				uint8_t serialized[24] = { 0 };

		// 				chunk->Serialize(serialized);

		// 				SECTION("compare serialized SdesChunk with original buffer")
		// 				{
		// 					REQUIRE(std::memcmp(chunkBuffer2 + 4 + 24, serialized, 24) == 0);
		// 				}
		// 			}

		// 			break;
		// 		}
		// 	}
		// }

		// // There are 2 chunks.
		// REQUIRE(chunkIdx == 1);

		delete packet;
	}

	SECTION("create SDES packet with more than 31 chunks")
	{
		const size_t count = 33;

		SdesPacket packet;

		for (auto i = 1; i <= count; i++)
		{
			// Create chunk and add to packet.
			SdesChunk* chunk1 = new SdesChunk(i /*ssrc*/);

			auto* item1 =
			  new RTC::RTCP::SdesItem(SdesItem::Type::CNAME, item1Value.size(), item1Value.c_str());

			chunk1->AddItem(item1);

			packet.AddChunk(chunk1);
		}

		REQUIRE(packet.GetCount() == count);

		uint8_t buffer1[1500] = { 0 };

		// Serialization must contain 2 RR packets since report count exceeds 31.
		packet.Serialize(buffer1);

		auto* packet2 = static_cast<SdesPacket*>(Packet::Parse(buffer1, sizeof(buffer1)));

		REQUIRE(packet2 != nullptr);
		REQUIRE(packet2->GetCount() == 31);

		auto reportIt = packet2->Begin();

		for (auto i = 1; i <= 31; i++, reportIt++)
		{
			auto* chunk = *reportIt;

			REQUIRE(chunk->GetSsrc() == i);

			auto* item = *(chunk->Begin());

			REQUIRE(item->GetType() == SdesItem::Type::CNAME);
			REQUIRE(item->GetSize() == 2 + item1Value.size());
			REQUIRE(std::string(item->GetValue()) == item1Value);
		}

		SdesPacket* packet3 = static_cast<SdesPacket*>(packet2->GetNext());

		REQUIRE(packet3 != nullptr);
		REQUIRE(packet3->GetCount() == 2);

		reportIt = packet3->Begin();

		for (auto i = 1; i <= 2; i++, reportIt++)
		{
			auto* chunk = *reportIt;

			REQUIRE(chunk->GetSsrc() == 31 + i);

			auto* item = *(chunk->Begin());

			REQUIRE(item->GetType() == SdesItem::Type::CNAME);
			REQUIRE(item->GetSize() == 2 + item1Value.size());
			REQUIRE(std::string(item->GetValue()) == item1Value);
		}

		delete packet2;
		delete packet3;
	}

	SECTION("create SdesChunk")
	{
		auto* item = new SdesItem(item1Type, item1Length, item1Value.c_str());

		// Create sdes chunk.
		SdesChunk chunk(ssrc1);

		chunk.AddItem(item);

		REQUIRE(chunk.GetSsrc() == ssrc1);

		SdesItem* item1 = *(chunk.Begin());

		REQUIRE(item1->GetType() == item1Type);
		REQUIRE(item1->GetLength() == item1Length);
		REQUIRE(std::string(item1->GetValue(), item1Length) == item1Value);
	}
}
