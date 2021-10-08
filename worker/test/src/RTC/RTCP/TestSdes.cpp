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

	SECTION("create SdesChunk")
	{
		SdesItem* item = new SdesItem(type, length, value.c_str());

		// Create sdes chunk.
		SdesChunk chunk(ssrc);

		chunk.AddItem(item);

		verify(&chunk);
	}
}
