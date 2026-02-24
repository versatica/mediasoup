#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP I-Data Chunk (64)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

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
		REQUIRE(chunk->GetStreamIdentifier() == 5001);
		REQUIRE(chunk->GetMessageIdentifier() == 1234567890);
		// Bit B is set so we have PPID instead of FSN.
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 99887766);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 0);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0xAB);
		REQUIRE(chunk->GetUserData()[1] == 0xCD);
		REQUIRE(chunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

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
		REQUIRE(chunk->GetStreamIdentifier() == 5001);
		REQUIRE(chunk->GetMessageIdentifier() == 1234567890);
		// Bit B is set so we have PPID instead of FSN.
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 99887766);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 0);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0xAB);
		REQUIRE(chunk->GetUserData()[1] == 0xCD);
		REQUIRE(chunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

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
		REQUIRE(chunk->GetStreamIdentifier() == 5001);
		REQUIRE(chunk->GetMessageIdentifier() == 1234567890);
		// Bit B is set so we have PPID instead of FSN.
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 99887766);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 0);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0xAB);
		REQUIRE(chunk->GetUserData()[1] == 0xCD);
		REQUIRE(chunk->GetUserData()[2] == 0xEF);
		// This should be padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);
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
		REQUIRE(chunk->GetStreamIdentifier() == 0);
		REQUIRE(chunk->GetMessageIdentifier() == 0);
		// Bit B is not set so we don't have PPID but FSN.
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 0);
		REQUIRE(chunk->HasUserData() == false);
		REQUIRE(chunk->GetUserDataLength() == 0);

		/* Modify it. */

		chunk->SetI(true);
		chunk->SetE(true);
		chunk->SetTsn(12345678);
		chunk->SetStreamIdentifier(9988);
		chunk->SetMessageIdentifier(1234);
		chunk->SetFragmentSequenceNumber(987654321);

		// Bit B is not set so cannot set PPID.
		REQUIRE_THROWS_AS(chunk->SetPayloadProtocolIdentifier(1234), MediaSoupError);

		// Verify that replacing the value works.
		chunk->SetUserData(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(chunk->GetLength() == 3020);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3000);

		chunk->SetUserData(nullptr, 0);

		REQUIRE(chunk->GetLength() == 20);
		REQUIRE(chunk->HasUserData() == false);
		REQUIRE(chunk->GetUserDataLength() == 0);

		// 3 bytes + 1 byte of padding.
		chunk->SetUserData(sctpCommon::DataBuffer, 3);

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
		REQUIRE(chunk->GetStreamIdentifier() == 9988);
		REQUIRE(chunk->GetMessageIdentifier() == 1234);
		// Bit B is not set so we don't have PPID but FSN.
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 987654321);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0x00);
		REQUIRE(chunk->GetUserData()[1] == 0x01);
		REQUIRE(chunk->GetUserData()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);

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
		REQUIRE(chunk->GetStreamIdentifier() == 9988);
		REQUIRE(chunk->GetMessageIdentifier() == 1234);
		// Bit B is not set so we don't have PPID but FSN.
		REQUIRE(chunk->GetPayloadProtocolIdentifier() == 0);
		REQUIRE(chunk->GetFragmentSequenceNumber() == 987654321);
		REQUIRE(chunk->HasUserData() == true);
		REQUIRE(chunk->GetUserDataLength() == 3);
		REQUIRE(chunk->GetUserData()[0] == 0x00);
		REQUIRE(chunk->GetUserData()[1] == 0x01);
		REQUIRE(chunk->GetUserData()[2] == 0x02);
		// Last byte must be a zero byte padding.
		REQUIRE(chunk->GetUserData()[3] == 0x00);
	}

	SECTION("IDataChunk::SetUserData() throws if userDataLength is too big")
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

		REQUIRE_THROWS_AS(chunk->SetUserData(sctpCommon::DataBuffer, 65535), MediaSoupError);

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
