#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("SCTP Payload Data Chunk (0)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("DataChunk::Parse() succeeds")
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
			// User Data (3 bytes): 0xABCDEF, 1 byte of padding
			0xAB, 0xCD, 0xEF, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = DataChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetB() == true);
		REQUIRE(chunk->GetE() == true);
		REQUIRE(chunk->GetTsn() == 0x11223344);
		REQUIRE(chunk->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0xAB);
		REQUIRE(chunk->GetUserData()[1] == 0xCD);
		REQUIRE(chunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(chunk->SetI(true), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetE(true), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetTsn(12345678), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetStreamIdentifierS(9988), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetStreamSequenceNumberN(2211), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetPayloadProtocolIdentifier(987654321), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetUserData(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetB() == true);
		REQUIRE(chunk->GetE() == true);
		REQUIRE(chunk->GetTsn() == 0x11223344);
		REQUIRE(chunk->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0xAB);
		REQUIRE(chunk->GetUserData()[1] == 0xCD);
		REQUIRE(chunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001011,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetI() == true);
		REQUIRE(clonedChunk->GetU() == false);
		REQUIRE(clonedChunk->GetB() == true);
		REQUIRE(clonedChunk->GetE() == true);
		REQUIRE(clonedChunk->GetTsn() == 0x11223344);
		REQUIRE(clonedChunk->GetStreamIdentifierS() == 0xFF00);
		REQUIRE(clonedChunk->GetStreamSequenceNumberN() == 0x6677);
		REQUIRE(clonedChunk->GetPayloadProtocolIdentifier() == 0x12341234);
		REQUIRE(clonedChunk->HasUserData() == true);
		REQUIRE(clonedChunk->GetUserDataLength() == 3);
		REQUIRE(clonedChunk->GetUserData()[0] == 0xAB);
		REQUIRE(clonedChunk->GetUserData()[1] == 0xCD);
		REQUIRE(clonedChunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(clonedChunk->GetUserData()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("DataChunk::Factory() succeeds")
	{
		auto* chunk = DataChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetB() == false);
		REQUIRE(chunk->GetE() == false);
		REQUIRE(chunk->GetTsn() == 0);
		REQUIRE(chunk->GetStreamIdentifierS() == 0);
		REQUIRE(chunk->GetStreamSequenceNumberN() == 0);
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0);
		REQUIRE(chunk->HasUserData() == false);
		REQUIRE(chunk->GetUserDataLength() == 0);

		/* Modify it. */

		chunk->SetI(true);
		chunk->SetE(true);
		chunk->SetTsn(12345678);
		chunk->SetStreamIdentifierS(9988);
		chunk->SetStreamSequenceNumberN(2211);
		chunk->SetPayloadProtocolIdentifier(987654321);

		// Verify that replacing the value works.
		chunk->SetUserData(DataBuffer + 1000, 3000);

		REQUIRE(chunk->GetLength() == 3016);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3000);

		chunk->SetUserData(nullptr, 0);

		REQUIRE(chunk->GetLength() == 16);
		REQUIRE(chunk->HasUserData() == false);
		REQUIRE(chunk->GetUserDataLength() == 0);

		// 3 bytes + 1 byte of padding.
		chunk->SetUserData(DataBuffer, 3);

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 16 + 3 + 1,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetB() == false);
		REQUIRE(chunk->GetE() == true);
		REQUIRE(chunk->GetTsn() == 12345678);
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

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 16 + 3 + 1,
		  /*length*/ 16 + 3 + 1,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00001001,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->GetI() == true);
		REQUIRE(parsedChunk->GetU() == false);
		REQUIRE(parsedChunk->GetB() == false);
		REQUIRE(parsedChunk->GetE() == true);
		REQUIRE(parsedChunk->GetTsn() == 12345678);
		REQUIRE(parsedChunk->GetStreamIdentifierS() == 9988);
		REQUIRE(parsedChunk->GetStreamSequenceNumberN() == 2211);
		REQUIRE(parsedChunk->GetPayloadProtocolIdentifier() == 987654321);
		REQUIRE(parsedChunk->HasUserData() == true);
		REQUIRE(parsedChunk->GetUserDataLength() == 3);
		REQUIRE(parsedChunk->GetUserData()[0] == 0x00);
		REQUIRE(parsedChunk->GetUserData()[1] == 0x01);
		REQUIRE(parsedChunk->GetUserData()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(parsedChunk->GetUserData()[3] == 0x00);

		delete parsedChunk;
	}

	SECTION("DataChunk::SetUserData() throws if userDataLength is too big")
	{
		auto* chunk = DataChunk::Factory(ThrowBuffer, sizeof(ThrowBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE_THROWS_AS(chunk->SetUserData(DataBuffer, 65535), MediaSoupError);

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 16,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		delete chunk;
	}
}
