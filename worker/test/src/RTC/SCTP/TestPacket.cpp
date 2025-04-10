#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/SCTP/DataChunk.hpp"
#include "RTC/SCTP/Packet.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("parse SCTP Packet without Chunks", "[sctp][serializable]")
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

SCENARIO("parse SCTP Packet with Chunks", "[sctp][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Source Port: 10000, Destination Port: 15999
		0x27, 0x10, 0x3E, 0x7F,
		// Verification Tag: 4294967285
		0xFF, 0xFF, 0xFF, 0xF5,
		// Checksum: 5
		0x00, 0x00, 0x00, 0x05,
		// Chunk 1: Type:0 (DATA), I:1, U:0, B:1, E:1, Length: 18
		0x00, 0b00001011, 0x00, 0x12,
		// TSN: 0x11223344,
		0x11, 0x22, 0x33, 0x44,
		// Stream Identifier S: 0xFF00, Stream Sequence Number n: 0x6677
		0xFF, 0x00, 0x66, 0x77,
		// Payload Protocol Identifier: 0x12341234
		0x12, 0x34, 0x12, 0x34,
		// User Data (2 bytes): 0xABCD, 2 bytes of padding.
		0xAB, 0xCD, 0x00, 0x00
	};
	// clang-format on

	REQUIRE(Packet::IsPacket(buffer, sizeof(buffer)) == true);

	auto* packet = Packet::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 32);
	REQUIRE(packet);
	REQUIRE(packet->GetBuffer() == buffer);
	REQUIRE(packet->GetBufferLength() == 32);
	REQUIRE(packet->GetLength() == 32);
	REQUIRE(packet->IsFrozen() == true);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->GetSourcePort() == 10000);
	REQUIRE(packet->GetDestinationPort() == 15999);
	REQUIRE(packet->GetVerificationTag() == 4294967285);
	REQUIRE(packet->GetChecksum() == 5);
	REQUIRE(packet->HasChunks() == true);
	REQUIRE(packet->GetChunksCount() == 1);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, 32) == true);

	auto* chunk1 = reinterpret_cast<const DataChunk*>(packet->GetChunkAt(0));

	REQUIRE(chunk1);
	REQUIRE(chunk1->GetBuffer() == buffer + 12);
	REQUIRE(chunk1->GetBufferLength() == 20);
	REQUIRE(chunk1->GetLength() == 20);
	REQUIRE(chunk1->IsFrozen() == true);
	REQUIRE(chunk1->GetType() == Chunk::ChunkType::DATA);
	REQUIRE(chunk1->HasUnknownType() == false);
	REQUIRE(chunk1->GetFlags() == 0b00001011);
	REQUIRE(chunk1->GetI() == true);
	REQUIRE(chunk1->GetU() == false);
	REQUIRE(chunk1->GetI() == true);
	REQUIRE(chunk1->GetI() == true);
	REQUIRE(chunk1->GetTSN() == 0x11223344);
	REQUIRE(chunk1->GetStreamIdentifierS() == 0xFF00);
	REQUIRE(chunk1->GetStreamSequenceNumberN() == 0x6677);
	REQUIRE(chunk1->GetPayloadProtocolIdentifier() == 0x12341234);
	REQUIRE(chunk1->HasUserData() == true);
	REQUIRE(chunk1->GetUserDataLength() == 2);
	REQUIRE(chunk1->GetUserData()[0] == 0xAB);
	REQUIRE(chunk1->GetUserData()[1] == 0xCD);
	REQUIRE(helpers::areBuffersEqual(chunk1->GetBuffer(), chunk1->GetLength(), buffer + 12, 20) == true);

	REQUIRE(!packet->GetChunkAt(1));

	delete packet;
}

SCENARIO("create and modify SCTP Packet without Chunks", "[sctp][serializable]")
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
