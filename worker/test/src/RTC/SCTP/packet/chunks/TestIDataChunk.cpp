#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("SCTP I-Data Chunk (64)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("IDataChunk::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:64 (I_DATA), I:1, U:0, B:1, E:0, Length: 23
			0x40, 0b00001010, 0x00, 0x17,
			// TSN: 0x11223344,
			0x11, 0x22, 0x33, 0x44,
			// Stream Identifier: 5001
			0x13, 0x89, 0x00, 0x00,
			// Message Identifier: 1234567890
			0x49, 0x96, 0x02, 0xD2,
			// Payload Protocol Identifier / Fragment Sequence Number: 99887766
			0x05, 0xF4, 0x2A, 0x96,
			// User Data (3 bytes): 0xABCDED, 1 byte of padding
			0xAB, 0xCD, 0xEF, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = IDataChunk::Parse(buffer, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
		  /*flags*/ 0b00001010,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetB() == true);
		REQUIRE(chunk->GetE() == false);
		REQUIRE(chunk->GetTsn() == 0x11223344);
		REQUIRE(chunk->GetStreamIdentifier() == 5001);
		REQUIRE(chunk->GetMessageIdentifier() == 1234567890);
		REQUIRE(chunk->GetPayloadProtocolIdentifierOrFragmentSequenceNumber() == 99887766);
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
		REQUIRE_THROWS_AS(chunk->SetStreamIdentifier(9988), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetMessageIdentifier(1234), MediaSoupError);
		REQUIRE_THROWS_AS(
		  chunk->SetPayloadProtocolIdentifierOrFragmentSequenceNumber(987654321), MediaSoupError);
		REQUIRE_THROWS_AS(chunk->SetUserData(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
		  /*flags*/ 0b00001010,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetI() == true);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetB() == true);
		REQUIRE(chunk->GetE() == false);
		REQUIRE(chunk->GetTsn() == 0x11223344);
		REQUIRE(chunk->GetStreamIdentifier() == 5001);
		REQUIRE(chunk->GetMessageIdentifier() == 1234567890);
		REQUIRE(chunk->GetPayloadProtocolIdentifierOrFragmentSequenceNumber() == 99887766);
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
		  /*length*/ 24,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
		  /*flags*/ 0b00001010,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->GetI() == true);
		REQUIRE(clonedChunk->GetU() == false);
		REQUIRE(clonedChunk->GetB() == true);
		REQUIRE(clonedChunk->GetE() == false);
		REQUIRE(clonedChunk->GetTsn() == 0x11223344);
		REQUIRE(clonedChunk->GetStreamIdentifier() == 5001);
		REQUIRE(clonedChunk->GetMessageIdentifier() == 1234567890);
		REQUIRE(clonedChunk->GetPayloadProtocolIdentifierOrFragmentSequenceNumber() == 99887766);
		REQUIRE(clonedChunk->HasUserData() == true);
		REQUIRE(clonedChunk->GetUserDataLength() == 3);
		REQUIRE(clonedChunk->GetUserData()[0] == 0xAB);
		REQUIRE(clonedChunk->GetUserData()[1] == 0xCD);
		REQUIRE(clonedChunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(clonedChunk->GetUserData()[3] == 0x00);

		delete clonedChunk;
	}

	SECTION("IDataChunk::Factory() succeeds")
	{
		auto* chunk = IDataChunk::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetU() == false);
		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetI() == false);
		REQUIRE(chunk->GetTsn() == 0);
		REQUIRE(chunk->GetStreamIdentifier() == 0);
		REQUIRE(chunk->GetMessageIdentifier() == 0);
		REQUIRE(chunk->GetPayloadProtocolIdentifierOrFragmentSequenceNumber() == 0);
		REQUIRE(chunk->HasUserData() == false);
		REQUIRE(chunk->GetUserDataLength() == 0);

		/* Modify it. */

		chunk->SetI(true);
		chunk->SetE(true);
		chunk->SetTsn(12345678);
		chunk->SetStreamIdentifier(9988);
		chunk->SetMessageIdentifier(1234);
		chunk->SetPayloadProtocolIdentifierOrFragmentSequenceNumber(987654321);

		// Verify that replacing the value works.
		chunk->SetUserData(DataBuffer + 1000, 3000);

		REQUIRE(chunk->GetLength() == 3020);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3000);

		chunk->SetUserData(nullptr, 0);

		REQUIRE(chunk->GetLength() == 20);
		REQUIRE(chunk->HasUserData() == false);
		REQUIRE(chunk->GetUserDataLength() == 0);

		// 3 bytes + 1 byte of padding.
		chunk->SetUserData(DataBuffer, 3);

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 20 + 3 + 1,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		REQUIRE(chunk->GetStreamIdentifier() == 9988);
		REQUIRE(chunk->GetMessageIdentifier() == 1234);
		REQUIRE(chunk->GetPayloadProtocolIdentifierOrFragmentSequenceNumber() == 987654321);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0x00);
		REQUIRE(chunk->GetUserData()[1] == 0x01);
		REQUIRE(chunk->GetUserData()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedChunk = IDataChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 20 + 3 + 1,
		  /*length*/ 20 + 3 + 1,
		  /*frozen*/ true,
		  /*chunkType*/ Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		REQUIRE(parsedChunk->GetStreamIdentifier() == 9988);
		REQUIRE(parsedChunk->GetMessageIdentifier() == 1234);
		REQUIRE(parsedChunk->GetPayloadProtocolIdentifierOrFragmentSequenceNumber() == 987654321);
		REQUIRE(parsedChunk->HasUserData() == true);
		REQUIRE(parsedChunk->GetUserDataLength() == 3);
		REQUIRE(parsedChunk->GetUserData()[0] == 0x00);
		REQUIRE(parsedChunk->GetUserData()[1] == 0x01);
		REQUIRE(parsedChunk->GetUserData()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(parsedChunk->GetUserData()[3] == 0x00);

		delete parsedChunk;
	}

	SECTION("IDataChunk::SetUserData() throws if userDataLength is too big")
	{
		auto* chunk = IDataChunk::Factory(ThrowBuffer, sizeof(ThrowBuffer));

		CHECK_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*chunkType*/ Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		delete chunk;
	}
}
