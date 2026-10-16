#include "common.hpp"
#include "RTC/RTP/Codecs/RED.hpp"
#include "RTC/RTP/Packet.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memcpy(), std::memcmp()

SCENARIO("RED payload", "[rtp][codecs][red]")
{
	// clang-format off
	// Two redundant blocks (PT 111) and the primary block (PT 111).
	const uint8_t redPayload[] =
	{
		// Block header 1: F=1, PT=111, ts offset=1920, block length=3.
		0xEF, 0x1E, 0x00, 0x03,
		// Block header 2: F=1, PT=111, ts offset=960, block length=2.
		0xEF, 0x0F, 0x00, 0x02,
		// Final block header: F=0, PT=111.
		0x6F,
		// Block 1 data.
		0x11, 0x12, 0x13,
		// Block 2 data.
		0x21, 0x22,
		// Primary block data.
		0x31, 0x32, 0x33, 0x34
	};
	// clang-format on

	SECTION("GetPrimaryBlock() succeeds")
	{
		uint8_t payloadType{ 0 };
		size_t offset{ 0 };

		REQUIRE(
		  RTC::RTP::Codecs::RED::GetPrimaryBlock(redPayload, sizeof(redPayload), payloadType, offset));
		REQUIRE(payloadType == 111);
		REQUIRE(offset == 14);
	}

	SECTION("GetPrimaryBlock() succeeds with just the primary block")
	{
		const uint8_t payload[] = { 0x6F, 0x31, 0x32 };
		uint8_t payloadType{ 0 };
		size_t offset{ 0 };

		REQUIRE(RTC::RTP::Codecs::RED::GetPrimaryBlock(payload, sizeof(payload), payloadType, offset));
		REQUIRE(payloadType == 111);
		REQUIRE(offset == 1);
	}

	SECTION("GetPrimaryBlock() fails with invalid payloads")
	{
		uint8_t payloadType{ 0 };
		size_t offset{ 0 };

		// Empty payload.
		REQUIRE_FALSE(RTC::RTP::Codecs::RED::GetPrimaryBlock(redPayload, 0, payloadType, offset));

		// Truncated non final block header.
		const uint8_t payload1[] = { 0xEF, 0x1E, 0x00 };

		REQUIRE_FALSE(
		  RTC::RTP::Codecs::RED::GetPrimaryBlock(payload1, sizeof(payload1), payloadType, offset));

		// Missing final block header.
		const uint8_t payload2[] = { 0xEF, 0x1E, 0x00, 0x00 };

		REQUIRE_FALSE(
		  RTC::RTP::Codecs::RED::GetPrimaryBlock(payload2, sizeof(payload2), payloadType, offset));

		// Block length exceeds the payload length.
		const uint8_t payload3[] = { 0xEF, 0x1E, 0x00, 0x05, 0x6F, 0x11, 0x12 };

		REQUIRE_FALSE(
		  RTC::RTP::Codecs::RED::GetPrimaryBlock(payload3, sizeof(payload3), payloadType, offset));
	}

	SECTION("MapPayloadTypes() succeeds")
	{
		uint8_t payload[sizeof(redPayload)];

		std::memcpy(payload, redPayload, sizeof(redPayload));

		const ankerl::unordered_dense::map<uint8_t, uint8_t> mapping{
			{ 111, 100 },
      { 63,  101 }
		};

		REQUIRE(RTC::RTP::Codecs::RED::MapPayloadTypes(payload, sizeof(payload), mapping));

		// F bits and the rest of the payload must be kept.
		REQUIRE(payload[0] == (0x80 | 100));
		REQUIRE(payload[4] == (0x80 | 100));
		REQUIRE(payload[8] == 100);
		REQUIRE(std::memcmp(payload + 1, redPayload + 1, 3) == 0);
		REQUIRE(std::memcmp(payload + 5, redPayload + 5, 3) == 0);
		REQUIRE(std::memcmp(payload + 9, redPayload + 9, sizeof(redPayload) - 9) == 0);
	}

	SECTION("MapPayloadTypes() fails and doesn't modify the payload if a payload type is unknown")
	{
		// clang-format off
		uint8_t payload[] =
		{
			// Block header: F=1, PT=0, ts offset=960, block length=1.
			0x80, 0x0F, 0x00, 0x01,
			// Final block header: F=0, PT=111.
			0x6F,
			0x11,
			0x31
		};
		// clang-format on
		uint8_t original[sizeof(payload)];

		std::memcpy(original, payload, sizeof(payload));

		const ankerl::unordered_dense::map<uint8_t, uint8_t> mapping{
			{ 111, 100 }
		};

		REQUIRE_FALSE(RTC::RTP::Codecs::RED::MapPayloadTypes(payload, sizeof(payload), mapping));
		REQUIRE(std::memcmp(payload, original, sizeof(payload)) == 0);
	}

	SECTION("Unwrap() succeeds")
	{
		alignas(4) uint8_t buffer[256];

		std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(buffer, sizeof(buffer)) };

		packet->SetPayloadType(63);
		packet->SetSequenceNumber(1234);
		packet->SetTimestamp(5678);
		packet->SetSsrc(1111);
		packet->SetPayload(redPayload, sizeof(redPayload));

		REQUIRE(RTC::RTP::Codecs::RED::Unwrap(packet.get()));

		const uint8_t expectedPayload[] = { 0x31, 0x32, 0x33, 0x34 };

		REQUIRE(packet->GetPayloadType() == 111);
		REQUIRE(packet->GetSequenceNumber() == 1234);
		REQUIRE(packet->GetTimestamp() == 5678);
		REQUIRE(packet->GetSsrc() == 1111);
		REQUIRE(packet->GetPayloadLength() == sizeof(expectedPayload));
		REQUIRE(std::memcmp(packet->GetPayload(), expectedPayload, sizeof(expectedPayload)) == 0);
		REQUIRE(packet->GetLength() == 12 + sizeof(expectedPayload));
	}

	SECTION("Unwrap() fails and doesn't modify the packet if the RED payload is invalid")
	{
		alignas(4) uint8_t buffer[256];

		std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(buffer, sizeof(buffer)) };

		const uint8_t payload[] = { 0xEF, 0x1E, 0x00, 0x05, 0x6F, 0x11, 0x12 };

		packet->SetPayloadType(63);
		packet->SetPayload(payload, sizeof(payload));

		REQUIRE_FALSE(RTC::RTP::Codecs::RED::Unwrap(packet.get()));
		REQUIRE(packet->GetPayloadType() == 63);
		REQUIRE(packet->GetPayloadLength() == sizeof(payload));
		REQUIRE(std::memcmp(packet->GetPayload(), payload, sizeof(payload)) == 0);
	}
}
