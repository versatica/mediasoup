#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Unrecognized Chunk Type Error Cause (6)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnrecognizedChunkTypeErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:6 (UNRECOGNIZED_CHUNK_TYPE), Length: 10
			0x00, 0x06, 0x00, 0x0A,
			// Unrecognized Chunk: 0x1234567890AB
			0x12, 0x34, 0x56, 0x78,
			// 2 bytes of padding.
			0x90, 0xAB, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC,
		};
		// clang-format on

		auto* errorCause = UnrecognizedChunkTypeErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedChunk() == true);
		REQUIRE(errorCause->GetUnrecognizedChunkLength() == 6);
		REQUIRE(errorCause->GetUnrecognizedChunk()[0] == 0x12);
		REQUIRE(errorCause->GetUnrecognizedChunk()[1] == 0x34);
		REQUIRE(errorCause->GetUnrecognizedChunk()[2] == 0x56);
		REQUIRE(errorCause->GetUnrecognizedChunk()[3] == 0x78);
		REQUIRE(errorCause->GetUnrecognizedChunk()[4] == 0x90);
		REQUIRE(errorCause->GetUnrecognizedChunk()[5] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause->GetUnrecognizedChunk()[6] == 0x00);
		REQUIRE(errorCause->GetUnrecognizedChunk()[7] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetUnrecognizedChunk(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedChunk() == true);
		REQUIRE(errorCause->GetUnrecognizedChunkLength() == 6);
		REQUIRE(errorCause->GetUnrecognizedChunk()[0] == 0x12);
		REQUIRE(errorCause->GetUnrecognizedChunk()[1] == 0x34);
		REQUIRE(errorCause->GetUnrecognizedChunk()[2] == 0x56);
		REQUIRE(errorCause->GetUnrecognizedChunk()[3] == 0x78);
		REQUIRE(errorCause->GetUnrecognizedChunk()[4] == 0x90);
		REQUIRE(errorCause->GetUnrecognizedChunk()[5] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause->GetUnrecognizedChunk()[6] == 0x00);
		REQUIRE(errorCause->GetUnrecognizedChunk()[7] == 0x00);

		/* Clone it. */

		auto* clonedErrorCause = errorCause->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ clonedErrorCause,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->HasUnrecognizedChunk() == true);
		REQUIRE(clonedErrorCause->GetUnrecognizedChunkLength() == 6);
		REQUIRE(clonedErrorCause->GetUnrecognizedChunk()[0] == 0x12);
		REQUIRE(clonedErrorCause->GetUnrecognizedChunk()[1] == 0x34);
		REQUIRE(clonedErrorCause->GetUnrecognizedChunk()[2] == 0x56);
		REQUIRE(clonedErrorCause->GetUnrecognizedChunk()[3] == 0x78);
		REQUIRE(clonedErrorCause->GetUnrecognizedChunk()[4] == 0x90);
		REQUIRE(clonedErrorCause->GetUnrecognizedChunk()[5] == 0xAB);
		// These should be padding.
		REQUIRE(clonedErrorCause->GetUnrecognizedChunk()[6] == 0x00);
		REQUIRE(clonedErrorCause->GetUnrecognizedChunk()[7] == 0x00);

		delete clonedErrorCause;
	}

	SECTION("UnrecognizedChunkTypeErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Unrecognized Chunk: 0x12345678
			0x12, 0x34, 0x56, 0x78,
		};
		// clang-format on

		REQUIRE(!UnrecognizedChunkTypeErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:6 (UNRECOGNIZED_CHUNK_TYPE), Length: 7
			0x00, 0x06, 0x00, 0x07,
			// Unrecognized Chunk: 0x123456 (missing padding byte)
			0x12, 0x34, 0x56,
		};
		// clang-format on

		REQUIRE(!UnrecognizedChunkTypeErrorCause::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("UnrecognizedChunkTypeErrorCause::Factory() succeeds")
	{
		auto* errorCause = UnrecognizedChunkTypeErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedChunk() == false);
		REQUIRE(errorCause->GetUnrecognizedChunkLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetUnrecognizedChunk(DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasUnrecognizedChunk() == true);
		REQUIRE(errorCause->GetUnrecognizedChunkLength() == 3000);

		errorCause->SetUnrecognizedChunk(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasUnrecognizedChunk() == false);
		REQUIRE(errorCause->GetUnrecognizedChunkLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetUnrecognizedChunk(DataBuffer, 6);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedChunk() == true);
		REQUIRE(errorCause->GetUnrecognizedChunkLength() == 6);
		REQUIRE(errorCause->GetUnrecognizedChunk()[0] == 0x00);
		REQUIRE(errorCause->GetUnrecognizedChunk()[1] == 0x01);
		REQUIRE(errorCause->GetUnrecognizedChunk()[2] == 0x02);
		REQUIRE(errorCause->GetUnrecognizedChunk()[3] == 0x03);
		REQUIRE(errorCause->GetUnrecognizedChunk()[4] == 0x04);
		REQUIRE(errorCause->GetUnrecognizedChunk()[5] == 0x05);
		// These should be padding.
		REQUIRE(errorCause->GetUnrecognizedChunk()[6] == 0x00);
		REQUIRE(errorCause->GetUnrecognizedChunk()[7] == 0x00);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  UnrecognizedChunkTypeErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->HasUnrecognizedChunk() == true);
		REQUIRE(parsedErrorCause->GetUnrecognizedChunkLength() == 6);
		REQUIRE(parsedErrorCause->GetUnrecognizedChunk()[0] == 0x00);
		REQUIRE(parsedErrorCause->GetUnrecognizedChunk()[1] == 0x01);
		REQUIRE(parsedErrorCause->GetUnrecognizedChunk()[2] == 0x02);
		REQUIRE(parsedErrorCause->GetUnrecognizedChunk()[3] == 0x03);
		REQUIRE(parsedErrorCause->GetUnrecognizedChunk()[4] == 0x04);
		REQUIRE(parsedErrorCause->GetUnrecognizedChunk()[5] == 0x05);
		// These should be padding.
		REQUIRE(parsedErrorCause->GetUnrecognizedChunk()[6] == 0x00);
		REQUIRE(parsedErrorCause->GetUnrecognizedChunk()[7] == 0x00);

		delete parsedErrorCause;
	}
}
