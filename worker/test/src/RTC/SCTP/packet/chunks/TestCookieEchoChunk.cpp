#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("SCTP Cookie Echo Chunk (10)", "[serializable][sctp][chunk]")
{
	sctpCommon::ResetBuffers();

	SECTION("CookieEchoChunk::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:A (COOKIE_ECHO), Flags: 0b00000000, Length: 9
			0x0A, 0b00000000, 0x00, 0x09,
			// Cookie: 0x1122334455,
			0x11, 0x22, 0x33, 0x44,
			// 3 bytes of padding
			0x55, 0x00, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
		};
		// clang-format on

		auto* chunk = RTC::SCTP::CookieEchoChunk::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->HasCookie() == true);
		REQUIRE(chunk->GetCookieLength() == 5);
		REQUIRE(chunk->GetCookie()[0] == 0x11);
		REQUIRE(chunk->GetCookie()[1] == 0x22);
		REQUIRE(chunk->GetCookie()[2] == 0x33);
		REQUIRE(chunk->GetCookie()[3] == 0x44);
		REQUIRE(chunk->GetCookie()[4] == 0x55);
		// This should be padding.
		REQUIRE(chunk->GetCookie()[5] == 0x00);
		REQUIRE(chunk->GetCookie()[6] == 0x00);
		REQUIRE(chunk->GetCookie()[7] == 0x00);

		/* Serialize it. */

		chunk->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->HasCookie() == true);
		REQUIRE(chunk->GetCookieLength() == 5);
		REQUIRE(chunk->GetCookie()[0] == 0x11);
		REQUIRE(chunk->GetCookie()[1] == 0x22);
		REQUIRE(chunk->GetCookie()[2] == 0x33);
		REQUIRE(chunk->GetCookie()[3] == 0x44);
		REQUIRE(chunk->GetCookie()[4] == 0x55);
		// This should be padding.
		REQUIRE(chunk->GetCookie()[5] == 0x00);
		REQUIRE(chunk->GetCookie()[6] == 0x00);
		REQUIRE(chunk->GetCookie()[7] == 0x00);

		/* Clone it. */

		auto* clonedChunk = chunk->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ clonedChunk,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(clonedChunk->HasCookie() == true);
		REQUIRE(clonedChunk->GetCookieLength() == 5);
		REQUIRE(clonedChunk->GetCookie()[0] == 0x11);
		REQUIRE(clonedChunk->GetCookie()[1] == 0x22);
		REQUIRE(clonedChunk->GetCookie()[2] == 0x33);
		REQUIRE(clonedChunk->GetCookie()[3] == 0x44);
		REQUIRE(clonedChunk->GetCookie()[4] == 0x55);
		// This should be padding.
		REQUIRE(clonedChunk->GetCookie()[5] == 0x00);
		REQUIRE(clonedChunk->GetCookie()[6] == 0x00);
		REQUIRE(clonedChunk->GetCookie()[7] == 0x00);

		delete clonedChunk;
	}

	SECTION("CookieEchoChunk::Factory() succeeds")
	{
		auto* chunk = RTC::SCTP::CookieEchoChunk::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->HasCookie() == false);
		REQUIRE(chunk->GetCookieLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		chunk->SetCookie(sctpCommon::DataBuffer + 1000, 2999);

		REQUIRE(chunk->GetLength() == 3004);
		REQUIRE(chunk->HasCookie() == true);
		REQUIRE(chunk->GetCookieLength() == 2999);

		chunk->SetCookie(nullptr, 0);

		REQUIRE(chunk->GetLength() == 4);
		REQUIRE(chunk->HasCookie() == false);
		REQUIRE(chunk->GetCookieLength() == 0);

		// 3 bytes + 1 byte of padding.
		chunk->SetCookie(sctpCommon::DataBuffer, 3);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(chunk->HasCookie() == true);
		REQUIRE(chunk->GetCookieLength() == 3);
		REQUIRE(chunk->GetCookie()[0] == 0x00);
		REQUIRE(chunk->GetCookie()[1] == 0x01);
		REQUIRE(chunk->GetCookie()[2] == 0x02);
		// This should be padding.
		REQUIRE(chunk->GetCookie()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedChunk = RTC::SCTP::CookieEchoChunk::Parse(chunk->GetBuffer(), chunk->GetLength());

		delete chunk;

		CHECK_SCTP_CHUNK(
		  /*chunk*/ parsedChunk,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE(parsedChunk->HasCookie() == true);
		REQUIRE(parsedChunk->GetCookieLength() == 3);
		REQUIRE(parsedChunk->GetCookie()[0] == 0x00);
		REQUIRE(parsedChunk->GetCookie()[1] == 0x01);
		REQUIRE(parsedChunk->GetCookie()[2] == 0x02);
		// This should be padding.
		REQUIRE(parsedChunk->GetCookie()[3] == 0x00);

		delete parsedChunk;
	}

	SECTION("CookieEchoChunk::SetCookie() throws if userDataLength is too big")
	{
		auto* chunk =
		  RTC::SCTP::CookieEchoChunk::Factory(sctpCommon::ThrowBuffer, sizeof(sctpCommon::ThrowBuffer));

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::ThrowBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::ThrowBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO,
		  /*unknownType*/ false,
		  /*actionForUnknownChunkType*/ RTC::SCTP::Chunk::ActionForUnknownChunkType::STOP,
		  /*flags*/ 0b00000000,
		  /*canHaveParameters*/ false,
		  /*parametersCount*/ 0,
		  /*canHaveErrorCauses*/ false,
		  /*errorCausesCount*/ 0);

		REQUIRE_THROWS_AS(chunk->SetCookie(sctpCommon::ThrowBuffer, 65535), MediaSoupError);

		CHECK_SCTP_CHUNK(
		  /*chunk*/ chunk,
		  /*buffer*/ sctpCommon::ThrowBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::ThrowBuffer),
		  /*length*/ 4,
		  /*chunkType*/ RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO,
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
