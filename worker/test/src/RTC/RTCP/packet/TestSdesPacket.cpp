#include "common.hpp"
#include "RTC/RTCP/packet/SdesPacket.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <string>

SCENARIO("RTCP SDES Packet (202)", "[serializable][rtcp][packet]")
{
	rtcpCommon::ResetBuffers();

	SECTION("SdesPacket::Parse() succeeds")
	{
		const auto checkItems = [](const RTC::RTCP::SdesPacket* packet) -> void
		{
			const auto items = packet->GetItems();

			REQUIRE(packet->GetNumberOfChunks() == 3);
			REQUIRE(items.size() == 3);

			REQUIRE(items[0].ssrc == 0x9F65E742);
			REQUIRE(items[0].type == RTC::RTCP::SdesPacket::ItemType::CNAME);
			REQUIRE(items[0].value == "t7mkYnCm46OcINy/");

			REQUIRE(items[1].ssrc == 1234);
			REQUIRE(items[1].type == RTC::RTCP::SdesPacket::ItemType::CNAME);
			REQUIRE(items[1].value == "qwerty");

			REQUIRE(items[2].ssrc == 1234);
			REQUIRE(items[2].type == RTC::RTCP::SdesPacket::ItemType::TOOL);
			REQUIRE(items[2].value == "iñaki");
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, SC=3, PT:202, Length: 14
			0b10000011, 0xCA, 0x00, 0x0E,
			// Chunk 1
			// SSRC: 0x9F65E742
			0x9F, 0x65, 0xE7, 0x42,
			// Item type: 1 (CNAME), Length: 16, Value: "t7mkYnCm46OcINy/"
			0x01, 0x10, 0x74, 0x37,
			0x6D, 0x6B, 0x59, 0x6E,
			0x43, 0x6D, 0x34, 0x36,
			0x4F, 0x63, 0x49, 0x4E,
			// End of value "y/", null octet and 1 octet of padding
			0x79, 0x2F, 0x00, 0x00,
			// Chunk 2
			// SSRC: 1234
			0x00, 0x00, 0x04, 0xD2,
			// Item type: 1 (CNAME), Length: 6, Value: "qwerty"
			0x01, 0x06, 0x71, 0x77,
			0x65, 0x72, 0x74, 0x79,
			// Item type: 6 (TOOL), Length: 6, Value: "iñaki"
			0x06, 0x06, 0x69, 0xC3,
			0xB1, 0x61, 0x6B, 0x69,
			// 4 null octets
			0x00, 0x00, 0x00, 0x00,
			// Chunk 3
			// SSRC: 5678
			0x00, 0x00, 0x16, 0x2E,
			// 4 null octets (no items)
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::SdesPacket> packet{ RTC::RTCP::SdesPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 60,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkItems(packet.get());

		// No space to add more chunks.
		REQUIRE_THROWS_AS(packet->AddChunk(3333), MediaSoupTypeError);
		// No space to add more items.
		REQUIRE_THROWS_AS(
		  packet->AddItem(RTC::RTCP::SdesPacket::ItemType::NAME, "foo"), MediaSoupTypeError);

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 60,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkItems(packet.get());

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 60,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkItems(packet.get());

		/* Modify it. */

		// Add an item to the last chunk (which has no items).
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::LOC, "ab");

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 64,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		const auto items = packet->GetItems();

		REQUIRE(packet->GetNumberOfChunks() == 3);
		REQUIRE(items.size() == 4);
		REQUIRE(items[3].ssrc == 5678);
		REQUIRE(items[3].type == RTC::RTCP::SdesPacket::ItemType::LOC);
		REQUIRE(items[3].value == "ab");
		// Check the null octet and padding bytes.
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 4] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 3] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 2] == 0);
		REQUIRE(packet->GetBuffer()[packet->GetLength() - 1] == 0);
	}

	SECTION("SdesPacket::Parse() with padding succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=1, SC=1, PT:202, Length: 4
			0b10100001, 0xCA, 0x00, 0x04,
			// Chunk 1
			// SSRC: 1234
			0x00, 0x00, 0x04, 0xD2,
			// Item type: 1 (CNAME), Length: 2, Value: "ab"
			0x01, 0x02, 0x61, 0x62,
			// 4 null octets
			0x00, 0x00, 0x00, 0x00,
			// Padding: 4 bytes
			0x00, 0x00, 0x00, 0x04,
			// Extra space in the buffer
			0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::SdesPacket> packet{ RTC::RTCP::SdesPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ true,
		  /*paddingLength*/ 4);

		REQUIRE(packet->GetNumberOfChunks() == 1);

		const auto items = packet->GetItems();

		REQUIRE(items.size() == 1);
		REQUIRE(items[0].ssrc == 1234);
		REQUIRE(items[0].type == RTC::RTCP::SdesPacket::ItemType::CNAME);
		REQUIRE(items[0].value == "ab");

		/* Modify it (it removes the padding). */

		packet->AddChunk(5678);

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetNumberOfChunks() == 2);
		REQUIRE(packet->GetItems().size() == 1);
		// The new chunk goes where the padding was.
		REQUIRE(Utils::Byte::Get4Bytes(packet->GetBuffer(), 16) == 5678);
		REQUIRE(Utils::Byte::Get4Bytes(packet->GetBuffer(), 20) == 0);
	}

	SECTION("SdesPacket::Parse() fails")
	{
		// SC field announces 2 chunks but there is only 1.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// V=2, P=0, SC=2, PT:202, Length: 2
			0b10000010, 0xCA, 0x00, 0x02,
			// SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// 4 null octets (no items)
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::SdesPacket::Parse(buffer1, sizeof(buffer1)));

		// Item length exceeds the packet length.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// V=2, P=0, SC=1, PT:202, Length: 2
			0b10000001, 0xCA, 0x00, 0x02,
			// SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// Item type: 1 (CNAME), Length: 20, Value: "ab"
			0x01, 0x14, 0x61, 0x62
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::SdesPacket::Parse(buffer2, sizeof(buffer2)));

		// Missing null octet terminating the items.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// V=2, P=0, SC=1, PT:202, Length: 2
			0b10000001, 0xCA, 0x00, 0x02,
			// SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// Item type: 1 (CNAME), Length: 2, Value: "ab"
			0x01, 0x02, 0x61, 0x62
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::SdesPacket::Parse(buffer3, sizeof(buffer3)));

		// Non null octet in the padding.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// V=2, P=0, SC=1, PT:202, Length: 3
			0b10000001, 0xCA, 0x00, 0x03,
			// SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// Item type: 1 (CNAME), Length: 3, Value: "abc"
			0x01, 0x03, 0x61, 0x62,
			// End of value "c", null octet and 2 octets of padding (the last one
			// non null)
			0x63, 0x00, 0x00, 0x01
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::SdesPacket::Parse(buffer4, sizeof(buffer4)));

		// Length field should be 2 (12 bytes) since the packet must end right
		// after the last chunk, but it announces 4 extra bytes.
		// clang-format off
		alignas(4) uint8_t buffer5[] =
		{
			// V=2, P=0, SC=1, PT:202, Length: 3
			0b10000001, 0xCA, 0x00, 0x03,
			// SSRC: 1111
			0x00, 0x00, 0x04, 0x57,
			// 4 null octets (no items)
			0x00, 0x00, 0x00, 0x00,
			// 4 extra bytes
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::SdesPacket::Parse(buffer5, sizeof(buffer5)));
	}

	SECTION("SdesPacket::Factory() succeeds")
	{
		const auto checkItems = [](const RTC::RTCP::SdesPacket* packet) -> void
		{
			const auto items = packet->GetItems();

			REQUIRE(packet->GetNumberOfChunks() == 2);
			REQUIRE(items.size() == 2);

			REQUIRE(items[0].ssrc == 1111);
			REQUIRE(items[0].type == RTC::RTCP::SdesPacket::ItemType::CNAME);
			REQUIRE(items[0].value == "foo");

			REQUIRE(items[1].ssrc == 2222);
			REQUIRE(items[1].type == RTC::RTCP::SdesPacket::ItemType::NOTE);
			REQUIRE(items[1].value == "bar baz");

			// Check the null octet and padding bytes of the first chunk.
			REQUIRE(packet->GetBuffer()[13] == 0);
			REQUIRE(packet->GetBuffer()[14] == 0);
			REQUIRE(packet->GetBuffer()[15] == 0);

			// Check the null octet and padding bytes of the second chunk.
			REQUIRE(packet->GetBuffer()[29] == 0);
			REQUIRE(packet->GetBuffer()[30] == 0);
			REQUIRE(packet->GetBuffer()[31] == 0);
		};

		std::unique_ptr<RTC::RTCP::SdesPacket> packet{ RTC::RTCP::SdesPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		REQUIRE(packet->GetNumberOfChunks() == 0);
		REQUIRE(packet->GetItems().empty());

		// There is no chunk to add the item to.
		REQUIRE_THROWS_AS(
		  packet->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, "foo"), MediaSoupTypeError);

		/* Modify it. */

		packet->AddChunk(1111);
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, "foo");
		packet->AddChunk(2222);
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::NOTE, "bar baz");

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkItems(packet.get());

		/* Serialize it. */

		packet->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(rtcpCommon::FactoryBuffer, 0xFF, sizeof(rtcpCommon::FactoryBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkItems(packet.get());

		/* Clone it. */

		packet.reset(packet->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 32,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkItems(packet.get());
	}

	SECTION("SdesPacket::AddChunk() and SdesPacket::AddItem() throw")
	{
		std::unique_ptr<RTC::RTCP::SdesPacket> packet{ RTC::RTCP::SdesPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		for (uint32_t ssrc{ 1 }; ssrc <= RTC::RTCP::SdesPacket::MaxChunks; ++ssrc)
		{
			packet->AddChunk(ssrc);
		}

		REQUIRE(packet->GetNumberOfChunks() == 31);
		REQUIRE(packet->GetLength() == 4 + (31 * 8));

		// No more than 31 chunks.
		REQUIRE_THROWS_AS(packet->AddChunk(32), MediaSoupTypeError);

		REQUIRE(packet->GetNumberOfChunks() == 31);
		REQUIRE(packet->GetLength() == 4 + (31 * 8));

		const std::string value255(255, 'a');
		const std::string value256(256, 'a');

		// No more than 255 bytes in the value.
		REQUIRE_THROWS_AS(
		  packet->AddItem(RTC::RTCP::SdesPacket::ItemType::NAME, value256), MediaSoupTypeError);

		REQUIRE(packet->GetItems().empty());
		REQUIRE(packet->GetLength() == 4 + (31 * 8));

		// Item type 0 is not allowed since it terminates the list of items.
		REQUIRE_THROWS_AS(
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  packet->AddItem(static_cast<RTC::RTCP::SdesPacket::ItemType>(0), "a"),
		  MediaSoupTypeError);

		REQUIRE(packet->GetItems().empty());
		REQUIRE(packet->GetLength() == 4 + (31 * 8));

		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::NAME, value255);

		const auto items = packet->GetItems();

		REQUIRE(items.size() == 1);
		REQUIRE(items[0].ssrc == 31);
		REQUIRE(items[0].value == value255);
		// 4 (header) + 30 empty chunks + 1 chunk with SSRC, item header, item
		// value and null octet padded to 4 bytes.
		REQUIRE(packet->GetLength() == 4 + (30 * 8) + 264);
	}

	SECTION("SdesPacket::AddItem() throws if there is no space in the buffer")
	{
		std::unique_ptr<RTC::RTCP::SdesPacket> packet{ RTC::RTCP::SdesPacket::Factory(
			rtcpCommon::FactoryBuffer, 12) };

		packet->AddChunk(1111);

		REQUIRE(packet->GetLength() == 12);

		// The item would need 16 bytes in total.
		REQUIRE_THROWS_AS(
		  packet->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, "abcd"), MediaSoupTypeError);

		REQUIRE(packet->GetLength() == 12);
		REQUIRE(packet->GetNumberOfChunks() == 1);
		REQUIRE(packet->GetItems().empty());
	}

	SECTION("SdesPacket with a chunk whose null octet ends at a 32-bit boundary")
	{
		const auto checkItems = [](const RTC::RTCP::SdesPacket* packet) -> void
		{
			const auto items = packet->GetItems();

			REQUIRE(packet->GetNumberOfChunks() == 2);
			REQUIRE(items.size() == 2);

			REQUIRE(items[0].ssrc == 0x11223344);
			REQUIRE(items[0].type == RTC::RTCP::SdesPacket::ItemType::LOC);
			REQUIRE(items[0].value == "abcdefghijklmnopq");

			REQUIRE(items[1].ssrc == 5678);
			REQUIRE(items[1].type == RTC::RTCP::SdesPacket::ItemType::CNAME);
			REQUIRE(items[1].value == "ab");
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// V=2, P=0, SC=2, PT:202, Length: 9
			0b10000010, 0xCA, 0x00, 0x09,
			// Chunk 1
			// SSRC: 0x11223344
			0x11, 0x22, 0x33, 0x44,
			// Item type: 5 (LOC), Length: 17, Value: "abcdefghijklmnopq"
			0x05, 0x11, 0x61, 0x62,
			0x63, 0x64, 0x65, 0x66,
			0x67, 0x68, 0x69, 0x6A,
			0x6B, 0x6C, 0x6D, 0x6E,
			// End of value "opq" and null octet (no padding)
			0x6F, 0x70, 0x71, 0x00,
			// Chunk 2
			// SSRC: 5678
			0x00, 0x00, 0x16, 0x2E,
			// Item type: 1 (CNAME), Length: 2, Value: "ab"
			0x01, 0x02, 0x61, 0x62,
			// Null octet and 3 octets of padding
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		/* Parse it. */

		const std::unique_ptr<RTC::RTCP::SdesPacket> parsedPacket{ RTC::RTCP::SdesPacket::Parse(
			buffer, sizeof(buffer)) };

		CHECK_RTCP_PACKET(
		  /*packet*/ parsedPacket.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 40,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkItems(parsedPacket.get());

		/* Build the same packet. */

		std::unique_ptr<RTC::RTCP::SdesPacket> packet{ RTC::RTCP::SdesPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		packet->AddChunk(0x11223344);
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::LOC, "abcdefghijklmnopq");

		REQUIRE(packet->GetLength() == 28);

		packet->AddChunk(5678);
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, "ab");

		CHECK_RTCP_PACKET(
		  /*packet*/ packet.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 40,
		  /*packetType*/ RTC::RTCP::Packet::PacketType::SDES,
		  /*unknownType*/ false,
		  /*hasPadding*/ false,
		  /*paddingLength*/ 0);

		checkItems(packet.get());

		REQUIRE(
		  helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, sizeof(buffer)) ==
		  true);
	}

	SECTION("SdesPacket::ComputeChunkLengthFor() succeeds")
	{
		// The SSRC/CSRC and the null octet, padded to 4 bytes.
		REQUIRE(RTC::RTCP::SdesPacket::ComputeChunkLengthFor({}) == 8);

		const std::unique_ptr<RTC::RTCP::SdesPacket> packet{ RTC::RTCP::SdesPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		const size_t lengthBefore = packet->GetLength();

		packet->AddChunk(1111);
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, "abc");
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::NAME, "de");

		// 4 + (2 + 3) + (2 + 2) + 1, padded to 4 bytes.
		REQUIRE(RTC::RTCP::SdesPacket::ComputeChunkLengthFor({ 3, 2 }) == 16);
		REQUIRE(
		  packet->GetLength() - lengthBefore ==
		  RTC::RTCP::SdesPacket::ComputeChunkLengthFor({ 3, 2 }));
	}

	SECTION("SdesPacket::ComputeLengthFor() succeeds")
	{
		REQUIRE(RTC::RTCP::SdesPacket::ComputeLengthFor(0) == 4);

		const std::unique_ptr<RTC::RTCP::SdesPacket> packet{ RTC::RTCP::SdesPacket::Factory(
			rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer)) };

		packet->AddChunk(1111);
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, "abc");
		packet->AddChunk(2222);
		packet->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, "defgh");

		const size_t length = RTC::RTCP::SdesPacket::ComputeLengthFor(
		  RTC::RTCP::SdesPacket::ComputeChunkLengthFor({ 3 }) +
		  RTC::RTCP::SdesPacket::ComputeChunkLengthFor({ 5 }));

		// 4 + (4 + 2 + 3 + 1 -> 12) + (4 + 2 + 5 + 1 -> 12).
		REQUIRE(length == 28);
		REQUIRE(packet->GetLength() == length);
	}
}
