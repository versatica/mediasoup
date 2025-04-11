#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "helpers.hpp"
#include "RTC/SCTP/DataChunk.hpp"
#include "RTC/SCTP/Packet.hpp"
#include "RTC/SCTP/ShutdownAckChunk.hpp"
#include "RTC/SCTP/ShutdownChunk.hpp"
#include "RTC/SCTP/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/UnknownChunk.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

void checkChunk(
  Chunk* chunk,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  Chunk::ChunkType chunkType,
  bool unknownType,
  uint8_t flags);

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
		// User Data (2 bytes): 0xABCD, 2 bytes of padding
		0xAB, 0xCD, 0x00, 0x00,
		// Chunk 2: Type:0xEE (UNKNOWN), Flags: 0b00001100, Length: 7
		0xEE, 0b00001100, 0x00, 0x07,
		// Unknown data: 0xAABBCC, 1 byte of padding
		0xAA, 0xBB, 0xCC, 0x00,
	};
	// clang-format on

	REQUIRE(Packet::IsPacket(buffer, sizeof(buffer)) == true);

	auto* packet = Packet::Parse(buffer, sizeof(buffer));

	REQUIRE(sizeof(buffer) == 40);
	REQUIRE(packet);
	REQUIRE(packet->GetBuffer() == buffer);
	REQUIRE(packet->GetBufferLength() == 40);
	REQUIRE(packet->GetLength() == 40);
	REQUIRE(packet->IsFrozen() == true);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->GetSourcePort() == 10000);
	REQUIRE(packet->GetDestinationPort() == 15999);
	REQUIRE(packet->GetVerificationTag() == 4294967285);
	REQUIRE(packet->GetChecksum() == 5);
	REQUIRE(packet->HasChunks() == true);
	REQUIRE(packet->GetChunksCount() == 2);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, 40) == true);

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

	auto* chunk2 = reinterpret_cast<const UnknownChunk*>(packet->GetChunkAt(1));

	REQUIRE(chunk2);
	REQUIRE(chunk2->GetBuffer() == buffer + 32);
	REQUIRE(chunk2->GetBufferLength() == 8);
	REQUIRE(chunk2->GetLength() == 8);
	REQUIRE(chunk2->IsFrozen() == true);
	REQUIRE(chunk2->GetType() == static_cast<Chunk::ChunkType>(0xEE));
	REQUIRE(chunk2->HasUnknownType() == true);
	REQUIRE(chunk2->GetFlags() == 0b00001100);
	REQUIRE(chunk2->HasUnknownData() == true);
	REQUIRE(chunk2->GetUnknownDataLength() == 3);
	REQUIRE(chunk2->GetUnknownData()[0] == 0xAA);
	REQUIRE(chunk2->GetUnknownData()[1] == 0xBB);
	REQUIRE(chunk2->GetUnknownData()[2] == 0xCC);
	REQUIRE(helpers::areBuffersEqual(chunk2->GetBuffer(), chunk2->GetLength(), buffer + 32, 8) == true);

	// Must throw if we try to modify chunks within the packet because they are
	// always frozen.
	REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetI(false), MediaSoupError);
	REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetU(false), MediaSoupError);
	REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetB(false), MediaSoupError);
	REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetE(false), MediaSoupError);
	REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetTSN(1234), MediaSoupError);
	REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetStreamIdentifierS(1234), MediaSoupError);
	REQUIRE_THROWS_AS(const_cast<DataChunk*>(chunk1)->SetStreamSequenceNumberN(1234), MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<DataChunk*>(chunk1)->SetPayloadProtocolIdentifier(1234), MediaSoupError);

	REQUIRE(!packet->GetChunkAt(2));

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

SCENARIO("create and modify SCTP Packet with Chunks", "[sctp][serializable]")
{
	uint8_t buffer[1000];
	uint8_t chunkBuffer[100];

	std::memset(buffer, 0xFF, sizeof(buffer));
	std::memset(chunkBuffer, 0xFF, sizeof(chunkBuffer));

	auto* packet = Packet::Factory(buffer, sizeof(buffer));

	REQUIRE(packet);

	/* Modify the packet. */

	packet->SetSourcePort(1024);
	packet->SetDestinationPort(2122);
	packet->SetVerificationTag(12345);
	packet->SetChecksum(99999);

	REQUIRE(packet->GetBuffer() == buffer);
	REQUIRE(packet->GetBufferLength() == 1000);
	REQUIRE(packet->GetLength() == 12);
	REQUIRE(packet->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->GetSourcePort() == 1024);
	REQUIRE(packet->GetDestinationPort() == 2122);
	REQUIRE(packet->GetVerificationTag() == 12345);
	REQUIRE(packet->GetChecksum() == 99999);
	REQUIRE(packet->HasChunks() == false);
	REQUIRE(packet->GetChunksCount() == 0);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, 12) == true);

	/* Add a DataChunk. */

	// UserData (3 bytes) so 1 byte of padding will be generted.
	uint8_t userData[] = { 0x01, 0x02, 0x03 };

	// Chunk 1 (16 + 3 + 1 = 20 bytes).
	auto* chunk1 = reinterpret_cast<DataChunk*>(packet->BuildChunkInPlace(Chunk::ChunkType::DATA));

	chunk1->SetI(true);
	chunk1->SetTSN(9876);
	chunk1->SetStreamIdentifierS(1234);
	chunk1->SetStreamSequenceNumberN(4321);
	chunk1->SetPayloadProtocolIdentifier(101010);
	chunk1->SetUserData(userData, sizeof(userData));

	REQUIRE(chunk1->IsFrozen() == false);

	// Before consolidating the Chunk, it must not be present in the Packet.
	REQUIRE(packet->HasChunks() == false);
	REQUIRE(packet->GetChunksCount() == 0);

	// Consolidate the Chunk.
	chunk1->Consolidate();

	REQUIRE(chunk1->GetBufferLength() == 20);
	REQUIRE(chunk1->GetLength() == 20);
	// It must be frozen after consolidating it.
	REQUIRE(chunk1->IsFrozen() == true);
	REQUIRE(chunk1->GetType() == Chunk::ChunkType::DATA);
	REQUIRE(chunk1->HasUnknownType() == false);
	REQUIRE(chunk1->GetFlags() == 0b00001000);
	REQUIRE(chunk1->GetI() == true);
	REQUIRE(chunk1->GetU() == false);
	REQUIRE(chunk1->GetB() == false);
	REQUIRE(chunk1->GetE() == false);
	REQUIRE(chunk1->GetTSN() == 9876);
	REQUIRE(chunk1->GetStreamIdentifierS() == 1234);
	REQUIRE(chunk1->GetStreamSequenceNumberN() == 4321);
	REQUIRE(chunk1->GetPayloadProtocolIdentifier() == 101010);
	REQUIRE(chunk1->HasUserData() == true);
	REQUIRE(chunk1->GetUserDataLength() == 3);
	REQUIRE(chunk1->GetUserData()[0] == 0x01);
	REQUIRE(chunk1->GetUserData()[1] == 0x02);
	REQUIRE(chunk1->GetUserData()[2] == 0x03);
	// Must be a padding byte.
	REQUIRE(chunk1->GetUserData()[3] == 0x00);

	// NOTE: Do not delete the Chunk since it's now part of the Packet.

	// Chunk 2 (8 bytes).
	auto* chunk2 =
	  reinterpret_cast<ShutdownChunk*>(packet->BuildChunkInPlace(Chunk::ChunkType::SHUTDOWN));

	chunk2->SetCumulativeTsnAck(1234567890);

	// Consolidate the Chunk.
	chunk2->Consolidate();

	REQUIRE(chunk2->GetBufferLength() == 8);
	REQUIRE(chunk2->GetLength() == 8);
	REQUIRE(chunk2->IsFrozen() == true);
	REQUIRE(chunk2->GetType() == Chunk::ChunkType::SHUTDOWN);
	REQUIRE(chunk2->HasUnknownType() == false);
	REQUIRE(chunk2->GetFlags() == 0b00000000);
	REQUIRE(chunk2->GetCumulativeTsnAck() == 1234567890);

	REQUIRE(packet->GetBuffer() == buffer);
	REQUIRE(packet->GetBufferLength() == 1000);
	REQUIRE(packet->GetLength() == 40);
	REQUIRE(packet->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->GetSourcePort() == 1024);
	REQUIRE(packet->GetDestinationPort() == 2122);
	REQUIRE(packet->GetVerificationTag() == 12345);
	REQUIRE(packet->GetChecksum() == 99999);
	REQUIRE(packet->HasChunks() == true);
	REQUIRE(packet->GetChunksCount() == 2);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, 40) == true);

	auto* addedChunk1 = reinterpret_cast<const DataChunk*>(packet->GetChunkAt(0));

	REQUIRE(addedChunk1->GetBufferLength() == 20);
	REQUIRE(addedChunk1->GetLength() == 20);
	// Internal chunks must always be frozen.
	REQUIRE(addedChunk1->IsFrozen() == true);
	REQUIRE(addedChunk1->GetType() == Chunk::ChunkType::DATA);
	REQUIRE(addedChunk1->HasUnknownType() == false);
	REQUIRE(addedChunk1->GetFlags() == 0b00001000);
	REQUIRE(addedChunk1->GetI() == true);
	REQUIRE(addedChunk1->GetU() == false);
	REQUIRE(addedChunk1->GetB() == false);
	REQUIRE(addedChunk1->GetE() == false);
	REQUIRE(addedChunk1->GetTSN() == 9876);
	REQUIRE(addedChunk1->GetStreamIdentifierS() == 1234);
	REQUIRE(addedChunk1->GetStreamSequenceNumberN() == 4321);
	REQUIRE(addedChunk1->GetPayloadProtocolIdentifier() == 101010);
	REQUIRE(addedChunk1->HasUserData() == true);
	REQUIRE(addedChunk1->GetUserDataLength() == 3);
	REQUIRE(addedChunk1->GetUserData()[0] == 0x01);
	REQUIRE(addedChunk1->GetUserData()[1] == 0x02);
	REQUIRE(addedChunk1->GetUserData()[2] == 0x03);

	auto* addedChunk2 = reinterpret_cast<const ShutdownChunk*>(packet->GetChunkAt(1));

	REQUIRE(addedChunk2->GetBufferLength() == 8);
	REQUIRE(addedChunk2->GetLength() == 8);
	REQUIRE(addedChunk2->IsFrozen() == true);
	REQUIRE(addedChunk2->GetType() == Chunk::ChunkType::SHUTDOWN);
	REQUIRE(addedChunk2->HasUnknownType() == false);
	REQUIRE(addedChunk2->GetFlags() == 0b00000000);
	REQUIRE(addedChunk2->GetCumulativeTsnAck() == 1234567890);

	/* Freeze Packet. */

	packet->Freeze();

	REQUIRE(packet->IsFrozen() == true);

	/* Serialize Packet into another buffer. */

	uint8_t newBuffer1[256];

	std::memset(newBuffer1, 0xFF, sizeof(newBuffer1));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(packet->Serialize(newBuffer1, packet->GetLength() - 1), MediaSoupTypeError);

	packet->Serialize(newBuffer1, sizeof(newBuffer1));

	// Compare new and old buffers.
	REQUIRE(helpers::areBuffersEqual(
	  packet->GetBuffer(), packet->GetLength(), buffer, packet->GetLength()));

	// Once done fill the old buffer with 1s.
	std::memset(buffer, 0xFF, sizeof(buffer));

	REQUIRE(packet->GetBuffer() == newBuffer1);
	REQUIRE(packet->GetBufferLength() == 256);
	REQUIRE(packet->GetLength() == 40);
	// After serializing, the packet must be unfrozen.
	REQUIRE(packet->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);
	REQUIRE(packet->GetSourcePort() == 1024);
	REQUIRE(packet->GetDestinationPort() == 2122);
	REQUIRE(packet->GetVerificationTag() == 12345);
	REQUIRE(packet->GetChecksum() == 99999);
	REQUIRE(packet->HasChunks() == true);
	REQUIRE(packet->GetChunksCount() == 2);
	REQUIRE(helpers::areBuffersEqual(packet->GetBuffer(), packet->GetLength(), newBuffer1, 40) == true);

	addedChunk1 = reinterpret_cast<const DataChunk*>(packet->GetChunkAt(0));

	REQUIRE(addedChunk1->GetBufferLength() == 20);
	REQUIRE(addedChunk1->GetLength() == 20);
	// After serializing, Chunks in the Packet must remain frozen.
	REQUIRE(addedChunk1->IsFrozen() == true);
	REQUIRE(addedChunk1->GetType() == Chunk::ChunkType::DATA);
	REQUIRE(addedChunk1->HasUnknownType() == false);
	REQUIRE(addedChunk1->GetFlags() == 0b00001000);
	REQUIRE(addedChunk1->GetI() == true);
	REQUIRE(addedChunk1->GetU() == false);
	REQUIRE(addedChunk1->GetB() == false);
	REQUIRE(addedChunk1->GetE() == false);
	REQUIRE(addedChunk1->GetTSN() == 9876);
	REQUIRE(addedChunk1->GetStreamIdentifierS() == 1234);
	REQUIRE(addedChunk1->GetStreamSequenceNumberN() == 4321);
	REQUIRE(addedChunk1->GetPayloadProtocolIdentifier() == 101010);
	REQUIRE(addedChunk1->HasUserData() == true);
	REQUIRE(addedChunk1->GetUserDataLength() == 3);
	REQUIRE(addedChunk1->GetUserData()[0] == 0x01);
	REQUIRE(addedChunk1->GetUserData()[1] == 0x02);
	REQUIRE(addedChunk1->GetUserData()[2] == 0x03);

	addedChunk2 = reinterpret_cast<const ShutdownChunk*>(packet->GetChunkAt(1));

	REQUIRE(addedChunk2->GetBufferLength() == 8);
	REQUIRE(addedChunk2->GetLength() == 8);
	// After serializing, Chunks in the Packet must remain frozen.
	REQUIRE(addedChunk2->IsFrozen() == true);
	REQUIRE(addedChunk2->GetType() == Chunk::ChunkType::SHUTDOWN);
	REQUIRE(addedChunk2->HasUnknownType() == false);
	REQUIRE(addedChunk2->GetFlags() == 0b00000000);
	REQUIRE(addedChunk2->GetCumulativeTsnAck() == 1234567890);

	/* Clone Packet into another buffer. */

	uint8_t newBuffer2[300];

	std::memset(newBuffer2, 0xFF, sizeof(newBuffer2));

	// Must throw if buffer is too small.
	REQUIRE_THROWS_AS(packet->Clone(newBuffer2, packet->GetLength() - 1), MediaSoupTypeError);

	auto* previousBuffer      = packet->GetBuffer();
	auto previousBufferLength = packet->GetBufferLength();
	auto* clonedPacket        = packet->Clone(newBuffer2, sizeof(newBuffer2));

	// Compare the buffers of the original Packet and the cloned one.
	REQUIRE(
	  helpers::areBuffersEqual(
	    clonedPacket->GetBuffer(), clonedPacket->GetLength(), newBuffer1, packet->GetLength()) == true);

	// Once done fill the original buffer with 1s (this is, we are ruining original
	// Packet despite it still exists since we have just cloned it).
	std::memset(const_cast<uint8_t*>(previousBuffer), 0xFF, previousBufferLength);

	// Freeze the original Packet again.
	packet->Freeze();

	REQUIRE(clonedPacket->GetBuffer() == newBuffer2);
	REQUIRE(clonedPacket->GetBufferLength() == 300);
	REQUIRE(clonedPacket->GetLength() == 40);
	// After cloning, the packet must be unfrozen.
	REQUIRE(clonedPacket->IsFrozen() == false);
	REQUIRE(Utils::Byte::IsPaddedTo4Bytes(clonedPacket->GetLength()) == true);
	REQUIRE(clonedPacket->GetSourcePort() == 1024);
	REQUIRE(clonedPacket->GetDestinationPort() == 2122);
	REQUIRE(clonedPacket->GetVerificationTag() == 12345);
	REQUIRE(clonedPacket->GetChecksum() == 99999);
	REQUIRE(clonedPacket->HasChunks() == true);
	REQUIRE(clonedPacket->GetChunksCount() == 2);
	REQUIRE(
	  helpers::areBuffersEqual(clonedPacket->GetBuffer(), clonedPacket->GetLength(), newBuffer2, 40) ==
	  true);

	addedChunk1 = reinterpret_cast<const DataChunk*>(clonedPacket->GetChunkAt(0));

	REQUIRE(addedChunk1->GetBufferLength() == 20);
	REQUIRE(addedChunk1->GetLength() == 20);
	// After cloning, Chunks in the Packet must remain frozen.
	REQUIRE(addedChunk1->IsFrozen() == true);
	REQUIRE(addedChunk1->GetType() == Chunk::ChunkType::DATA);
	REQUIRE(addedChunk1->HasUnknownType() == false);
	REQUIRE(addedChunk1->GetFlags() == 0b00001000);
	REQUIRE(addedChunk1->GetI() == true);
	REQUIRE(addedChunk1->GetU() == false);
	REQUIRE(addedChunk1->GetB() == false);
	REQUIRE(addedChunk1->GetE() == false);
	REQUIRE(addedChunk1->GetTSN() == 9876);
	REQUIRE(addedChunk1->GetStreamIdentifierS() == 1234);
	REQUIRE(addedChunk1->GetStreamSequenceNumberN() == 4321);
	REQUIRE(addedChunk1->GetPayloadProtocolIdentifier() == 101010);
	REQUIRE(addedChunk1->HasUserData() == true);
	REQUIRE(addedChunk1->GetUserDataLength() == 3);
	REQUIRE(addedChunk1->GetUserData()[0] == 0x01);
	REQUIRE(addedChunk1->GetUserData()[1] == 0x02);
	REQUIRE(addedChunk1->GetUserData()[2] == 0x03);

	addedChunk2 = reinterpret_cast<const ShutdownChunk*>(clonedPacket->GetChunkAt(1));

	REQUIRE(addedChunk2->GetBufferLength() == 8);
	REQUIRE(addedChunk2->GetLength() == 8);
	// After cloning, Chunks in the Packet must remain frozen.
	REQUIRE(addedChunk2->IsFrozen() == true);
	REQUIRE(addedChunk2->GetType() == Chunk::ChunkType::SHUTDOWN);
	REQUIRE(addedChunk2->HasUnknownType() == false);
	REQUIRE(addedChunk2->GetFlags() == 0b00000000);
	REQUIRE(addedChunk2->GetCumulativeTsnAck() == 1234567890);

	delete packet;
	delete clonedPacket;
}

SCENARIO("SCTP Payload Data Chunk (0)", "[sctp][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:0 (DATA), I:1, U:0, B:1, E:1, Length: 19
		0x00, 0b00001011, 0x00, 0x13,
		// TSN: 0x11223344,
		0x11, 0x22, 0x33, 0x44,
		// Stream Identifier S: 0xFF00, Stream Sequence Number n: 0x6677
		0xFF, 0x00, 0x66, 0x77,
		// Payload Protocol Identifier: 0x12341234
		0x12, 0x34, 0x12, 0x34,
		// User Data (2 bytes): 0xABCD, 1 byte of padding
		0xAB, 0xCD, 0xEF, 0x00,
		// Extra bytes that should be ignored.
		0xAA, 0xBB, 0xCC
	};
	// clang-format on

	auto* chunk = DataChunk::Parse(buffer, sizeof(buffer));

	// clang-format off
	checkChunk(
		/*chunk*/ chunk,
		/*buffer*/ buffer,
		/*bufferLength*/ sizeof(buffer),
		/*length*/ 20,
		/*frozen*/ true,
		/*chunkType*/ Chunk::ChunkType::DATA,
		/*unknownType*/ false,
		/*flags*/ 0b00001011
	);
	// clang-format on

	REQUIRE(chunk->GetI() == true);
	REQUIRE(chunk->GetU() == false);
	REQUIRE(chunk->GetI() == true);
	REQUIRE(chunk->GetI() == true);
	REQUIRE(chunk->GetTSN() == 0x11223344);
	REQUIRE(chunk->GetStreamIdentifierS() == 0xFF00);
	REQUIRE(chunk->GetStreamSequenceNumberN() == 0x6677);
	REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0x12341234);
	REQUIRE(chunk->HasUserData() == true);
	REQUIRE(chunk->GetUserDataLength() == 3);
	REQUIRE(chunk->GetUserData()[0] == 0xAB);
	REQUIRE(chunk->GetUserData()[1] == 0xCD);
	REQUIRE(chunk->GetUserData()[2] == 0xEF);

	delete chunk;
}

SCENARIO("SCTP Shutdown Association Chunk (7)", "[sctp][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:7 (SHUTDOWN), Flags:0x00000000, Length: 8
		0x07, 0b00000000, 0x00, 0x08,
		// Cumulative TSN Ack: 0x11223344,
		0x11, 0x22, 0x33, 0x44,
		// Extra bytes that should be ignored.
		0xAA, 0xBB, 0xCC
	};
	// clang-format on

	auto* chunk = ShutdownChunk::Parse(buffer, sizeof(buffer));

	// clang-format off
	checkChunk(
		/*chunk*/ chunk,
		/*buffer*/ buffer,
		/*bufferLength*/ sizeof(buffer),
		/*length*/ 8,
		/*frozen*/ true,
		/*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		/*unknownType*/ false,
		/*flags*/ 0b00000000
	);
	// clang-format on

	REQUIRE(chunk->GetCumulativeTsnAck() == 0x11223344);

	delete chunk;
}

SCENARIO("SCTP Shutdown Ack Chunk (8)", "[sctp][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:8 (SHUTDOWN_ACK), Flags:0x00000000, Length: 4
		0x08, 0b01000000, 0x00, 0x04,
		// Extra bytes that should be ignored.
		0xAA, 0xBB
	};
	// clang-format on

	auto* chunk = ShutdownAckChunk::Parse(buffer, sizeof(buffer));

	// clang-format off
	checkChunk(
		/*chunk*/ chunk,
		/*buffer*/ buffer,
		/*bufferLength*/ sizeof(buffer),
		/*length*/ 4,
		/*frozen*/ true,
		/*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		/*unknownType*/ false,
		/*flags*/ 0b01000000
	);
	// clang-format on

	delete chunk;
}

SCENARIO("SCTP Shutdown Complete Chunk (14)", "[sctp][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:8 (SHUTDOWN_COMPLETE), Flags:0x00000001, T: 1, Length: 4
		0x0E, 0b00000001, 0x00, 0x04,
		// Extra bytes that should be ignored.
		0xAA, 0xBB, 0xCC, 0xDD
	};
	// clang-format on

	auto* chunk = ShutdownCompleteChunk::Parse(buffer, sizeof(buffer));

	// clang-format off
	checkChunk(
		/*chunk*/ chunk,
		/*buffer*/ buffer,
		/*bufferLength*/ sizeof(buffer),
		/*length*/ 4,
		/*frozen*/ true,
		/*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		/*unknownType*/ false,
		/*flags*/ 0b00000001
	);
	// clang-format on

	REQUIRE(chunk->GetT() == true);

	delete chunk;
}

SCENARIO("SCTP Unknown Chunk (238)", "[sctp][serializable]")
{
	// clang-format off
	uint8_t buffer[] =
	{
		// Type:0xEE (UNKNOWN), Flags: 0b1100, Length: 7
		0xEE, 0b10001100, 0x00, 0x07,
		// Unknown data: 0xAABBCC, 1 byte of padding
		0xAA, 0xBB, 0xCC, 0x00,
		// Extra bytes that should be ignored.
		0xAA, 0xBB, 0xCC
	};
	// clang-format on

	auto* chunk = UnknownChunk::Parse(buffer, sizeof(buffer));

	// clang-format off
	checkChunk(
		/*chunk*/ chunk,
		/*buffer*/ buffer,
		/*bufferLength*/ sizeof(buffer),
		/*length*/ 8,
		/*frozen*/ true,
		/*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		/*unknownType*/ true,
		/*flags*/ 0b10001100
	);
	// clang-format on

	REQUIRE(chunk->HasUnknownData() == true);
	REQUIRE(chunk->GetUnknownDataLength() == 3);
	REQUIRE(chunk->GetUnknownData()[0] == 0xAA);
	REQUIRE(chunk->GetUnknownData()[1] == 0xBB);
	REQUIRE(chunk->GetUnknownData()[2] == 0xCC);

	delete chunk;
}

void checkChunk(
  Chunk* chunk,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  Chunk::ChunkType chunkType,
  bool unknownType,
  uint8_t flags)
{
	REQUIRE(chunk);
	REQUIRE(chunk->GetBufferLength() == bufferLength);
	REQUIRE(chunk->GetLength() == length);
	REQUIRE(chunk->IsFrozen() == frozen);
	REQUIRE(chunk->GetType() == chunkType);
	REQUIRE(chunk->HasUnknownType() == unknownType);
	REQUIRE(chunk->GetFlags() == flags);
	REQUIRE(helpers::areBuffersEqual(chunk->GetBuffer(), chunk->GetLength(), buffer, length) == true);
}
