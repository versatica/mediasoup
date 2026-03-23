#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()
#include <vector>

SCENARIO("SCTP I-Data Chunk (64)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("IDataChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:64 (I_DATA), I:1, U:0, B:1, E:0, Length: 23
			0x40, 0b00001010, 0x00, 0x17,
			// TSN: 0x11223344,
			0x11, 0x22, 0x33, 0x44,
			// Stream Identifier: 5001
			0x13, 0x89, 0x00, 0x00,
			// Message Identifier: 1234567890
			0x49, 0x96, 0x02, 0xD2,
			// Payload Protocol Identifier / Fragment Sequence Number: 99887766 (PPID)
			0x05, 0xF4, 0x2A, 0x96,
			// User Data (3 bytes): 0xABCDED, 1 byte of padding
			0xAB, 0xCD, 0xEF, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::IDataChunk> chunk{ RTC::SCTP::IDataChunk::Parse(
			buffer, sizeof(buffer)) };

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 24,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		REQUIRE(chunk->GetStreamId() == 5001);
		REQUIRE(chunk->GetMessageId() == 1234567890);
		// Bit B is set so we have PPID instead of FSN.
		REQUIRE(chunk->GetPayloadProtocolId() == 99887766);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 0);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3);
		REQUIRE(chunk->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(chunk->GetUserDataPayload()[1] == 0xCD);
		REQUIRE(chunk->GetUserDataPayload()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserDataPayload()[3] == 0x00);

		auto userData = chunk->GetUserData();

		std::vector<uint8_t> expectedPayload = { 0xAB, 0xCD, 0xEF };

		REQUIRE(userData.GetStreamId() == 5001);
		REQUIRE(userData.GetStreamSequenceNumber() == 0);
		REQUIRE(userData.GetMessageId() == 1234567890);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 99887766);
		// NOTE: clang-tidy doesn't understand that this is fine.
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		// Bit B is not set so cannot set FSN.
		REQUIRE_THROWS_AS(chunk->SetFragmentSequenceNumber(1234), MediaSoupError);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 24,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		REQUIRE(chunk->GetStreamId() == 5001);
		REQUIRE(chunk->GetMessageId() == 1234567890);
		// Bit B is set so we have PPID instead of FSN.
		REQUIRE(chunk->GetPayloadProtocolId() == 99887766);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 0);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3);
		REQUIRE(chunk->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(chunk->GetUserDataPayload()[1] == 0xCD);
		REQUIRE(chunk->GetUserDataPayload()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserDataPayload()[3] == 0x00);

		userData = chunk->GetUserData();

		REQUIRE(userData.GetStreamId() == 5001);
		REQUIRE(userData.GetStreamSequenceNumber() == 0);
		REQUIRE(userData.GetMessageId() == 1234567890);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 99887766);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		/* Clone it. */

		chunk.reset(chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 24,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		REQUIRE(chunk->GetStreamId() == 5001);
		REQUIRE(chunk->GetMessageId() == 1234567890);
		// Bit B is set so we have PPID instead of FSN.
		REQUIRE(chunk->GetPayloadProtocolId() == 99887766);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 0);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3);
		REQUIRE(chunk->GetUserDataPayload()[0] == 0xAB);
		REQUIRE(chunk->GetUserDataPayload()[1] == 0xCD);
		REQUIRE(chunk->GetUserDataPayload()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserDataPayload()[3] == 0x00);

		userData = chunk->GetUserData();

		REQUIRE(userData.GetStreamId() == 5001);
		REQUIRE(userData.GetStreamSequenceNumber() == 0);
		REQUIRE(userData.GetMessageId() == 1234567890);
		REQUIRE(userData.GetFragmentSequenceNumber() == 0);
		REQUIRE(userData.GetPayloadProtocolId() == 99887766);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);
	}

	SECTION("IDataChunk::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::IDataChunk> chunk{ RTC::SCTP::IDataChunk::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		REQUIRE(chunk->GetMessageId() == 0);
		// Bit B is not set so we don't have PPID but FSN.
		REQUIRE(chunk->GetPayloadProtocolId() == 0);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 0);
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
		chunk->SetMessageId(1234);
		chunk->SetFragmentSequenceNumber(987654321);

		// Bit B is not set so cannot set PPID.
		REQUIRE_THROWS_AS(chunk->SetPayloadProtocolId(1234), MediaSoupError);

		// Verify that replacing the value works.
		chunk->SetUserDataPayload(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(chunk->GetLength() == 3020);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3000);

		chunk->SetUserDataPayload(nullptr, 0);

		REQUIRE(chunk->GetLength() == 20);
		REQUIRE(chunk->HasUserDataPayload() == false);
		REQUIRE(chunk->GetUserDataPayloadLength() == 0);

		// 3 bytes + 1 byte of padding.
		chunk->SetUserDataPayload(sctpCommon::DataBuffer, 3);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 20 + 3 + 1,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		REQUIRE(chunk->GetMessageId() == 1234);
		// Bit B is not set so we don't have PPID but FSN.
		REQUIRE(chunk->GetPayloadProtocolId() == 0);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 987654321);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3);
		REQUIRE(chunk->GetUserDataPayload()[0] == 0x00);
		REQUIRE(chunk->GetUserDataPayload()[1] == 0x01);
		REQUIRE(chunk->GetUserDataPayload()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(chunk->GetUserDataPayload()[3] == 0x00);

		userData = chunk->GetUserData();

		expectedPayload = { 0x00, 0x01, 0x02 };

		REQUIRE(userData.GetStreamId() == 9988);
		REQUIRE(userData.GetStreamSequenceNumber() == 0);
		REQUIRE(userData.GetMessageId() == 1234);
		REQUIRE(userData.GetFragmentSequenceNumber() == 987654321);
		REQUIRE(userData.GetPayloadProtocolId() == 0);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);

		/* Parse itself and compare. */

		chunk.reset(RTC::SCTP::IDataChunk::Parse(chunk->GetBuffer(), chunk->GetLength()));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 20 + 3 + 1,
		  /*length*/ 20 + 3 + 1,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
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
		REQUIRE(chunk->GetMessageId() == 1234);
		// Bit B is not set so we don't have PPID but FSN.
		REQUIRE(chunk->GetPayloadProtocolId() == 0);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 987654321);
		REQUIRE(chunk->HasUserDataPayload() == true);
		REQUIRE(chunk->GetUserDataPayloadLength() == 3);
		REQUIRE(chunk->GetUserDataPayload()[0] == 0x00);
		REQUIRE(chunk->GetUserDataPayload()[1] == 0x01);
		REQUIRE(chunk->GetUserDataPayload()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(chunk->GetUserDataPayload()[3] == 0x00);

		userData = chunk->GetUserData();

		REQUIRE(userData.GetStreamId() == 9988);
		REQUIRE(userData.GetStreamSequenceNumber() == 0);
		REQUIRE(userData.GetMessageId() == 1234);
		REQUIRE(userData.GetFragmentSequenceNumber() == 987654321);
		REQUIRE(userData.GetPayloadProtocolId() == 0);
		// NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
		REQUIRE(std::move(userData).ReleasePayload() == expectedPayload);
	}

	SECTION("IDataChunk::SetUserDataPayload() throws if userDataPayloadLength is too big")
	{
		std::unique_ptr<RTC::SCTP::IDataChunk> chunk{ RTC::SCTP::IDataChunk::Factory(
			sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer)) };

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE_THROWS_AS(chunk->SetUserDataPayload(sctpCommon::DataBuffer, 65535), MediaSoupError);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::I_DATA,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP_AND_REPORT,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);
	}
}
