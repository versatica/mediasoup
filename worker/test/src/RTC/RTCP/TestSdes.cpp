#include "common.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include <catch2/catch.hpp>
#include <cstring> // std::memcmp()
#include <string>

using namespace RTC::RTCP;

namespace TestSdes
{
	// RTCP Sdes Packet.

	// clang-format off
	uint8_t buffer[] =
	{
		0x81, 0xca, 0x00, 0x06, // Type: 202 (SDES), Count: 1, Length: 6
		0x9f, 0x65, 0xe7, 0x42, // SSRC: 0x9f65e742
		0x01, 0x10, 0x74, 0x37, // Item Type: 1 (CNAME), Length: 16, Value: t7mkYnCm46OcINy/
		0x6d, 0x6b, 0x59, 0x6e,
		0x43, 0x6d, 0x34, 0x36,
		0x4f, 0x63, 0x49, 0x4e,
		0x79, 0x2f, 0x00, 0x00
	};
	// clang-format on

	uint8_t* chunkBuffer = buffer + Packet::CommonHeaderSize;

	// SDES values.
	uint32_t ssrc{ 0x9f65e742 };
	SdesItem::Type type{ SdesItem::Type::CNAME };
	std::string value{ "t7mkYnCm46OcINy/" };
	size_t length = 16;

	void verify(SdesChunk* chunk)
	{
		REQUIRE(chunk->GetSsrc() == ssrc);

		SdesItem* item = *(chunk->Begin());

		REQUIRE(item->GetType() == type);
		REQUIRE(item->GetLength() == length);
		REQUIRE(std::string(item->GetValue(), length) == value);
	}
} // namespace TestSdes

using namespace TestSdes;

SCENARIO("RTCP SDES parsing", "[parser][rtcp][sdes]")
{
	SECTION("parse packet")
	{
		SdesPacket* packet = SdesPacket::Parse(buffer, sizeof(buffer));
		SdesChunk* chunk   = *(packet->Begin());

		auto* header = reinterpret_cast<RTC::RTCP::Packet::CommonHeader*>(buffer);

		REQUIRE(ntohs(header->length) == 6);
		REQUIRE(chunk);

		verify(chunk);

		SECTION("serialize SdesChunk instance")
		{
			uint8_t serialized[sizeof(buffer) - Packet::CommonHeaderSize] = { 0 };

			chunk->Serialize(serialized);

			SECTION("compare serialized SdesChunk with original buffer")
			{
				REQUIRE(std::memcmp(chunkBuffer, serialized, sizeof(buffer) - Packet::CommonHeaderSize) == 0);
			}
		}
		delete packet;
	}

	SECTION("create SDES packet with more than 31 chunks")
	{
		const size_t count = 33;

		SdesPacket packet;

		for (auto i = 1; i <= count; i++)
		{
			// Create chunk and add to packet.
			SdesChunk* chunk = new SdesChunk(i /*ssrc*/);

			auto* item = new RTC::RTCP::SdesItem(SdesItem::Type::CNAME, value.size(), value.c_str());

			chunk->AddItem(item);

			packet.AddChunk(chunk);
		}

		REQUIRE(packet.GetCount() == count);

		uint8_t buffer[1500] = { 0 };

		// Serialization must contain 2 RR packets since report count exceeds 31.
		packet.Serialize(buffer);

		auto* packet2 = static_cast<SdesPacket*>(Packet::Parse(buffer, sizeof(buffer)));

		REQUIRE(packet2 != nullptr);
		REQUIRE(packet2->GetCount() == 31);

		auto reportIt = packet2->Begin();

		for (auto i = 1; i <= 31; i++, reportIt++)
		{
			auto* chunk = *reportIt;

			REQUIRE(chunk->GetSsrc() == i);

			auto* item = *(chunk->Begin());

			REQUIRE(item->GetType() == SdesItem::Type::CNAME);
			REQUIRE(item->GetSize() == 2 + value.size());
			REQUIRE(std::string(item->GetValue()) == value);
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
			REQUIRE(item->GetSize() == 2 + value.size());
			REQUIRE(std::string(item->GetValue()) == value);
		}

		delete packet2;
		delete packet3;
	}

	SECTION("create SdesChunk")
	{
		auto* item = new SdesItem(type, length, value.c_str());

		// Create sdes chunk.
		SdesChunk chunk(ssrc);

		chunk.AddItem(item);

		verify(&chunk);
	}
}
