#include "common.hpp"
#include "catch.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include <string>

using namespace RTC::RTCP;

SCENARIO("RTCP SDES parsing", "[parser][rtcp][sdes]")
{
	SECTION("parse SdesChunk")
	{
		uint8_t buffer[] =
		{
			0x00, 0x00, 0x00, 0x00, // SDES SSRC
			0x01, 0x0a, 0x6f, 0x75, // SDES Item
			0x74, 0x43, 0x68, 0x61,
			0x6e, 0x6e, 0x65, 0x6c
		};

		uint32_t ssrc       = 0;
		SdesItem::Type type = SdesItem::Type::CNAME;
		std::string value   = "outChannel";
		size_t len          = value.size();
		SdesChunk* chunk    = SdesChunk::Parse(buffer, sizeof(buffer));

		REQUIRE(chunk->GetSsrc() == ssrc);

		SdesItem* item = *(chunk->Begin());

		REQUIRE(item->GetType() == type);
		REQUIRE(item->GetLength() == len);
		REQUIRE(std::string(item->GetValue(), len) == "outChannel");

		delete chunk;
	}

	SECTION("create SdesChunk")
	{
		uint32_t ssrc       = 0;
		SdesItem::Type type = SdesItem::Type::CNAME;
		std::string value   = "outChannel";
		size_t len          = value.size();
		// Create sdes item.
		SdesItem* item = new SdesItem(type, len, value.c_str());

		// Create sdes chunk.
		SdesChunk chunk(ssrc);

		chunk.AddItem(item);

		// Check chunk content.
		REQUIRE(chunk.GetSsrc() == ssrc);

		// Check item content.
		item = *(chunk.Begin());

		REQUIRE(item->GetType() == type);
		REQUIRE(item->GetLength() == len);
		REQUIRE(std::string(item->GetValue(), len) == "outChannel");
	}
}
