#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/SCTP/Packet.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("parse empty SCTP Packet", "[sctp][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Source Port: 10000, Destination Port: 15999
		0x27, 0x10, 0x3E, 0x7F,
		// Verification Tag: 4294967285
		0xFF, 0xFF, 0xFF, 0xF5,
		// Checksum: 5
		0x00, 0x00, 0x00, 0x05
	};
	// clang-format on

	REQUIRE(Packet::IsPacket(buffer, sizeof(buffer)) == true);

	auto* packet = Packet::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 12);
	REQUIRE(packet);
	REQUIRE(packet->GetBuffer() == buffer);
	REQUIRE(packet->GetBufferLength() == 12);
	REQUIRE(packet->GetLength() == 12);
	REQUIRE(packet->IsFrozen() == true);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->GetSourcePort() == 10000);
	REQUIRE(packet->GetDestinationPort() == 15999);
	REQUIRE(packet->GetVerificationTag() == 4294967285);
	REQUIRE(packet->GetChecksum() == 5);
	REQUIRE(packet->HasChunks() == false);
	REQUIRE(packet->GetChunksCount() == 0);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, 12) == true);

	// Must throw if we try to modify the packet since Parse() returns a frozen
	// Packet.
	REQUIRE_THROWS_AS(packet->SetSourcePort(10), MediaSoupError);
	REQUIRE_THROWS_AS(packet->SetDestinationPort(9999), MediaSoupError);
	REQUIRE_THROWS_AS(packet->SetVerificationTag(12345), MediaSoupError);
	REQUIRE_THROWS_AS(packet->SetChecksum(6666), MediaSoupError);
	REQUIRE_THROWS_AS(packet->AddChunk(nullptr), MediaSoupError);

	delete packet;
}

SCENARIO("create and modify SCTP Packet", "[sctp][serializable]")
{
	uint8_t buffer[256];

	std::memset(buffer, 0xFF, sizeof(buffer));

	auto* packet = Packet::Factory(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 256);
	REQUIRE(packet);
	REQUIRE(packet->GetBuffer() == buffer);
	REQUIRE(packet->GetBufferLength() == 256);
	REQUIRE(packet->GetLength() == 12);
	REQUIRE(packet->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->GetSourcePort() == 0);
	REQUIRE(packet->GetDestinationPort() == 0);
	REQUIRE(packet->GetVerificationTag() == 0);
	REQUIRE(packet->GetChecksum() == 0);
	REQUIRE(packet->HasChunks() == false);
	REQUIRE(packet->GetChunksCount() == 0);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, 12) == true);

	/* Modify the packet. */

	packet->SetSourcePort(10);
	packet->SetDestinationPort(9999);
	packet->SetVerificationTag(12345);
	packet->SetChecksum(6666);

	REQUIRE(packet->GetBuffer() == buffer);
	REQUIRE(packet->GetBufferLength() == 256);
	REQUIRE(packet->GetLength() == 12);
	REQUIRE(packet->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->GetSourcePort() == 10);
	REQUIRE(packet->GetDestinationPort() == 9999);
	REQUIRE(packet->GetVerificationTag() == 12345);
	REQUIRE(packet->GetChecksum() == 6666);
	REQUIRE(packet->HasChunks() == false);
	REQUIRE(packet->GetChunksCount() == 0);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, 12) == true);

	delete packet;
}
