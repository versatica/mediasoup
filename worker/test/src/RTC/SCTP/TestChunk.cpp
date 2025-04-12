#include "common.hpp"
#include "helpers.hpp"
#include "RTC/SCTP/chunks/DataChunk.hpp"
#include "RTC/SCTP/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/chunks/ShutdownChunk.hpp"
#include "RTC/SCTP/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/chunks/UnknownChunk.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

thread_local static uint8_t ChunkFactoryBuffer[15000];
thread_local static uint8_t ChunkSerializeBuffer[15000];
thread_local static uint8_t ChunkCloneBuffer[15000];
thread_local static uint8_t ChunkCustomDataBuffer[15000];

static void checkChunk(
  Chunk* chunk,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  Chunk::ChunkType chunkType,
  bool unknownType,
  uint8_t flags);

static void resetBuffers();

SCENARIO("SCTP Payload Data Chunk (0)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("DataChunk::Parse()")
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

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*flags*/ 0b00001011);

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

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*flags*/ 0b00001011);

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

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*flags*/ 0b00001011);

		REQUIRE(clonedChunk->GetI() == true);
		REQUIRE(clonedChunk->GetU() == false);
		REQUIRE(clonedChunk->GetI() == true);
		REQUIRE(clonedChunk->GetI() == true);
		REQUIRE(clonedChunk->GetTSN() == 0x11223344);
		REQUIRE(clonedChunk->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(clonedChunk->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(clonedChunk->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(clonedChunk->HasUserData() == true);
		REQUIRE(clonedChunk->GetUserDataLength() == 3);
		REQUIRE(clonedChunk->GetUserData()[0] == 0xAB);
		REQUIRE(clonedChunk->GetUserData()[1] == 0xCD);
		REQUIRE(clonedChunk->GetUserData()[2] == 0xEF);

		delete clonedChunk;
	}

	SECTION("DataChunk::Factory()")
	{
		auto* chunk = DataChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetTSN() == 0);
		REQUIRE(chunk->GetStreamIdentifierS() == 0);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 0);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0);
		REQUIRE(chunk->HasUserData() == false);
		REQUIRE(chunk->GetUserDataLength() == 0);

		/* Modify it. */

		chunk->SetI(true);
		chunk->SetE(true);
		chunk->SetTSN(12345678);
		chunk->SetStreamIdentifierS(9988);
		chunk->SetStreamSequenceNumberN(2211);
		chunk->SetPayloadProtocolIdentifier(987654321);
		// 3 byres + 1 byte of padding.
		chunk->SetUserData(ChunkCustomDataBuffer, 3);

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 16 + 3 + 1,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*flags*/ 0b00001001);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetB() == false);
		REQUIRE(chunk->GetE() == true);
		REQUIRE(chunk->GetTSN() == 12345678);
		REQUIRE(chunk->GetStreamIdentifierS() == 9988);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 2211);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 987654321);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0x00);
		REQUIRE(chunk->GetUserData()[1] == 0x01);
		REQUIRE(chunk->GetUserData()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedChunk = DataChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ chunk->GetLength(),
		  /*length*/ 16 + 3 + 1,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*flags*/ 0b00001001);

		REQUIRE(parsedChunk->GetI() == true);
		REQUIRE(parsedChunk->GetU() == false);
		REQUIRE(parsedChunk->GetB() == false);
		REQUIRE(parsedChunk->GetE() == true);
		REQUIRE(parsedChunk->GetTSN() == 12345678);
		REQUIRE(parsedChunk->GetStreamIdentifierS() == 9988);
		REQUIRE(parsedChunk->GetStreamSequenceNumberN() == 2211);
		REQUIRE(parsedChunk->GetPayloadProtocolIdentifier() == 987654321);
		REQUIRE(parsedChunk->HasUserData() == true);
		REQUIRE(parsedChunk->GetUserDataLength() == 3);
		REQUIRE(parsedChunk->GetUserData()[0] == 0x00);
		REQUIRE(parsedChunk->GetUserData()[1] == 0x01);
		REQUIRE(parsedChunk->GetUserData()[2] == 0x02);
		REQUIRE(parsedChunk->GetUserData()[3] == 0x00);
		// Compare buffers.
		REQUIRE(
		  helpers::areBuffersEqual(
		    parsedChunk->GetBuffer(), parsedChunk->GetLength(), chunk->GetBuffer(), chunk->GetLength()) ==
		  true);

		delete chunk;
		delete parsedChunk;
	}
}

SCENARIO("SCTP Shutdown Association Chunk (7)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ShutdownChunk::Parse()")
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

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0x11223344);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0x11223344);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		REQUIRE(clonedChunk->GetCumulativeTsnAck() == 0x11223344);

		delete clonedChunk;
	}

	SECTION("ShutdownChunk::Factory()")
	{
		auto* chunk = ShutdownChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		REQUIRE(chunk->GetCumulativeTsnAck() == 0);

		/* Modify it. */

		chunk->SetCumulativeTsnAck(99887766);

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		REQUIRE(chunk->GetCumulativeTsnAck() == 99887766);

		/* Parse itself and compare. */

		auto* parsedChunk = ShutdownChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ chunk->GetLength(),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		REQUIRE(parsedChunk->GetCumulativeTsnAck() == 99887766);
		// Compare buffers.
		REQUIRE(
		  helpers::areBuffersEqual(
		    parsedChunk->GetBuffer(), parsedChunk->GetLength(), chunk->GetBuffer(), chunk->GetLength()) ==
		  true);

		delete chunk;
		delete parsedChunk;
	}
}

SCENARIO("SCTP Shutdown Ack Chunk (8)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ShutdownAckChunk::Parse()")
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

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*flags*/ 0b01000000);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*flags*/ 0b01000000);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*flags*/ 0b01000000);

		delete clonedChunk;
	}

	SECTION("ShutdownAckChunk::Factory()")
	{
		auto* chunk = ShutdownAckChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		/* Parse itself and compare. */

		auto* parsedChunk = ShutdownAckChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ chunk->GetLength(),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_ACK,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		// Compare buffers.
		REQUIRE(
		  helpers::areBuffersEqual(
		    parsedChunk->GetBuffer(), parsedChunk->GetLength(), chunk->GetBuffer(), chunk->GetLength()) ==
		  true);

		delete chunk;
		delete parsedChunk;
	}
}

SCENARIO("SCTP Shutdown Complete Chunk (14)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ShutdownCompleteChunk::Parse()")
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

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000001);

		REQUIRE(chunk->GetT() == true);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000001);

		REQUIRE(chunk->GetT() == true);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000001);

		REQUIRE(clonedChunk->GetT() == true);

		delete clonedChunk;
	}

	SECTION("ShutdownCompleteChunk::Factory()")
	{
		auto* chunk = ShutdownCompleteChunk::Factory(ChunkFactoryBuffer, sizeof(ChunkFactoryBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000000);

		REQUIRE(chunk->GetT() == false);

		/* Modify it. */

		chunk->SetT(true);

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000001);

		REQUIRE(chunk->GetT() == true);

		/* Parse itself and compare. */

		auto* parsedChunk = ShutdownCompleteChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		checkChunk(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ ChunkFactoryBuffer,
		  /*bufferLength*/ chunk->GetLength(),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::SHUTDOWN_COMPLETE,
		  /*unknownType*/ false,
		  /*flags*/ 0b00000001);

		REQUIRE(parsedChunk->GetT() == true);
		// Compare buffers.
		REQUIRE(
		  helpers::areBuffersEqual(
		    parsedChunk->GetBuffer(), parsedChunk->GetLength(), chunk->GetBuffer(), chunk->GetLength()) ==
		  true);

		delete chunk;
		delete parsedChunk;
	}
}

SCENARIO("SCTP Unknown Chunk (238)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnknownChunk::Parse()")
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

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*flags*/ 0b10001100);

		REQUIRE(chunk->HasUnknownData() == true);
		REQUIRE(chunk->GetUnknownDataLength() == 3);
		REQUIRE(chunk->GetUnknownData()[0] == 0xAA);
		REQUIRE(chunk->GetUnknownData()[1] == 0xBB);
		REQUIRE(chunk->GetUnknownData()[2] == 0xCC);

		/* Serialize it. */

		chunk->Serialize(ChunkSerializeBuffer, sizeof(ChunkSerializeBuffer));

		checkChunk(
		  /*chunk*/ chunk,
		  /*buffer*/ ChunkSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkSerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*flags*/ 0b10001100);

		REQUIRE(chunk->HasUnknownData() == true);
		REQUIRE(chunk->GetUnknownDataLength() == 3);
		REQUIRE(chunk->GetUnknownData()[0] == 0xAA);
		REQUIRE(chunk->GetUnknownData()[1] == 0xBB);
		REQUIRE(chunk->GetUnknownData()[2] == 0xCC);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(ChunkCloneBuffer, sizeof(ChunkCloneBuffer));

		delete chunk;

		checkChunk(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ ChunkCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkCloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*chunkType*/ static_cast<Chunk::ChunkType>(0xEE),
		  /*unknownType*/ true,
		  /*flags*/ 0b10001100);

		REQUIRE(clonedChunk->HasUnknownData() == true);
		REQUIRE(clonedChunk->GetUnknownDataLength() == 3);
		REQUIRE(clonedChunk->GetUnknownData()[0] == 0xAA);
		REQUIRE(clonedChunk->GetUnknownData()[1] == 0xBB);
		REQUIRE(clonedChunk->GetUnknownData()[2] == 0xCC);

		delete clonedChunk;
	}
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
	REQUIRE(chunk->GetBuffer() == buffer);
	REQUIRE(chunk->GetBufferLength() == bufferLength);
	REQUIRE(chunk->GetLength() == length);
	REQUIRE(chunk->IsFrozen() == frozen);
	REQUIRE(chunk->GetType() == chunkType);
	REQUIRE(chunk->HasUnknownType() == unknownType);
	REQUIRE(chunk->GetFlags() == flags);
	REQUIRE(helpers::areBuffersEqual(chunk->GetBuffer(), chunk->GetLength(), buffer, length) == true);
}

void resetBuffers()
{
	std::memset(ChunkFactoryBuffer, 0xAA, sizeof(ChunkFactoryBuffer));
	std::memset(ChunkSerializeBuffer, 0xBB, sizeof(ChunkSerializeBuffer));
	std::memset(ChunkCloneBuffer, 0xCC, sizeof(ChunkCloneBuffer));
	std::memset(ChunkCustomDataBuffer, 0xDD, sizeof(ChunkCustomDataBuffer));

	ChunkCustomDataBuffer[0] = 0x00;
	ChunkCustomDataBuffer[1] = 0x01;
	ChunkCustomDataBuffer[2] = 0x02;
	ChunkCustomDataBuffer[3] = 0x03;
	ChunkCustomDataBuffer[4] = 0x04;
	ChunkCustomDataBuffer[5] = 0x05;
	ChunkCustomDataBuffer[6] = 0x06;
	ChunkCustomDataBuffer[7] = 0x07;
}
