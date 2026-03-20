#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("SCTP Payload Data Chunk (0)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("DataChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* chunk = RTC::SCTP::DataChunk::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(chunk->GetStreamId() == 0xFF00);
		REQUIRE(chunk->GetStreamSequenceNumber() == 0x6677);
		REQUIRE(chunk->GetPayloadProtocolId() == 0x12341234);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3);
		REQUIRE(chunk->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(chunk->GetUserDataPayload()[1] == 0xCD);
		REQUIRE(chunk->GetUserDataPayload()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserDataPayload()[3] == 0x00);

		auto userData = chunk->GetUserData();

		std::vector<uint8_t> expectedPayload = { 0xAB, 0xCD, 0xEF };

		REQUIRE(userData.GetStreamId() == 0xFF00);
		REQUIRE(userData.GetStreamSequenceNumber() == 0x6677);
		REQUIRE(userData.GetMessageId() == 0);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 0x12341234);
		// NOTE: clang-tidy doesn't understand that this is fine.
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(chunk->GetStreamId() == 0xFF00);
		REQUIRE(chunk->GetStreamSequenceNumber() == 0x6677);
		REQUIRE(chunk->GetPayloadProtocolId() == 0x12341234);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3);
		REQUIRE(chunk->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(chunk->GetUserDataPayload()[1] == 0xCD);
		REQUIRE(chunk->GetUserDataPayload()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserDataPayload()[3] == 0x00);

		userData = chunk->GetUserData();

		REQUIRE(userData.GetStreamId() == 0xFF00);
		REQUIRE(userData.GetStreamSequenceNumber() == 0x6677);
		REQUIRE(userData.GetMessageId() == 0);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 0x12341234);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(clonedChunk->GetStreamId() == 0xFF00);
		REQUIRE(clonedChunk->GetStreamSequenceNumber() == 0x6677);
		REQUIRE(clonedChunk->GetPayloadProtocolId() == 0x12341234);
		REQUIRE(clonedChunk->HasUserDataPayload() == true);
		REQUIRE(clonedChunk->GetUserDataPayloadLength() == 3);
		REQUIRE(clonedChunk->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(clonedChunk->GetUserDataPayload()[1] == 0xCD);
		REQUIRE(clonedChunk->GetUserDataPayload()[2] == 0xEF);
		// This should be padding.
		REQUIRE(clonedChunk->GetUserDataPayload()[3] == 0x00);

		userData = clonedChunk->GetUserData();

		REQUIRE(userData.GetStreamId() == 0xFF00);
		REQUIRE(userData.GetStreamSequenceNumber() == 0x6677);
		REQUIRE(userData.GetMessageId() == 0);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 0x12341234);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		delete clonedChunk;
	}

	SECTION("DataChunk::Factory() succeeds")
	{
		auto* chunk =
		  RTC::SCTP::DataChunk::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 16,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(chunk->GetStreamId() == 0);
		REQUIRE(chunk->GetStreamSequenceNumber() == 0);
		REQUIRE(chunk->GetPayloadProtocolId() == 0);
		REQUIRE(chunk->HasUserDataPayload() == false);
		REQUIRE(chunk->GetUserDataPayloadLength() == 0);

		auto userData = chunk->GetUserData();

		std::vector<uint8_t> expectedPayload = {};

		REQUIRE(userData.GetStreamId() == 0);
		REQUIRE(userData.GetStreamSequenceNumber() == 0);
		REQUIRE(userData.GetMessageId() == 0);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 0);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		/* Modify it. */

		chunk->SetI(true);
		chunk->SetE(true);
		chunk->SetTsn(12345678);
		chunk->SetStreamId(9988);
		chunk->SetStreamSequenceNumber(2211);
		chunk->SetPayloadProtocolId(987654321);

		// Verify that replacing the value works.
		chunk->SetUserDataPayload(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(chunk->GetLength() == 3016);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3000);

		chunk->SetUserDataPayload(nullptr, 0);

		REQUIRE(chunk->GetLength() == 16);
		REQUIRE(chunk->HasUserDataPayload() == false);
		REQUIRE(chunk->GetUserDataPayloadLength() == 0);

		// 3 bytes + 1 byte of padding.
		chunk->SetUserDataPayload(sctpCommon::DataBuffer, 3);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 16 + 3 + 1,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(chunk->GetStreamId() == 9988);
		REQUIRE(chunk->GetStreamSequenceNumber() == 2211);
		REQUIRE(chunk->GetPayloadProtocolId() == 987654321);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3);
		REQUIRE(chunk->GetUserDataPayload()[0] == 0x00);
		REQUIRE(chunk->GetUserDataPayload()[1] == 0x01);
		REQUIRE(chunk->GetUserDataPayload()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(chunk->GetUserDataPayload()[3] == 0x00);

		userData        = chunk->GetUserData();
		expectedPayload = { 0x00, 0x01, 0x02 };

		REQUIRE(userData.GetStreamId() == 9988);
		REQUIRE(userData.GetStreamSequenceNumber() == 2211);
		REQUIRE(userData.GetMessageId() == 0);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 987654321);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		/* Parse itself and compare. */

		auto* parsedChunk = RTC::SCTP::DataChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 16 + 3 + 1,
		  /*length*/ 16 + 3 + 1,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
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
		REQUIRE(parsedChunk->GetStreamId() == 9988);
		REQUIRE(parsedChunk->GetStreamSequenceNumber() == 2211);
		REQUIRE(parsedChunk->GetPayloadProtocolId() == 987654321);
		REQUIRE(parsedChunk->HasUserDataPayload() == true);
		REQUIRE(parsedChunk->GetUserDataPayloadLength() == 3);
		REQUIRE(parsedChunk->GetUserDataPayload()[0] == 0x00);
		REQUIRE(parsedChunk->GetUserDataPayload()[1] == 0x01);
		REQUIRE(parsedChunk->GetUserDataPayload()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(parsedChunk->GetUserDataPayload()[3] == 0x00);

		userData = parsedChunk->GetUserData();

		REQUIRE(userData.GetStreamId() == 9988);
		REQUIRE(userData.GetStreamSequenceNumber() == 2211);
		REQUIRE(userData.GetMessageId() == 0);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 987654321);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		delete parsedChunk;
	}

	SECTION("DataChunk::SetUserDataPayload() throws if userDataLength is too big")
	{
		auto* chunk =
		  RTC::SCTP::DataChunk::Factory(sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 16,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE_THROWS_AS(chunk->SetUserDataPayload(sctpCommon::DataBuffer, 65535), MediaSoupError);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 16,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		delete chunk;
	}
}
