#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/InvalidStreamIdentifierErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Invalid Stream Identifier Error Cause (1)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("InvalidStreamIdentifierErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:1 (INVALID_STREAM_IDENTIFIER), Length: 8
			0x00, 0x01, 0x00, 0x08,
			// Stream Identifier: 12345
			0x30, 0x39, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* errorCause = InvalidStreamIdentifierErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetStreamIdentifier() == 12345);
		// Reserved bytes must be 0.
		REQUIRE(errorCause->GetBuffer()[6] == 0);
		REQUIRE(errorCause->GetBuffer()[7] == 0);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetStreamIdentifier(44444), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetStreamIdentifier() == 12345);
		// Reserved bytes must be 0.
		REQUIRE(errorCause->GetBuffer()[6] == 0);
		REQUIRE(errorCause->GetBuffer()[7] == 0);

		/* Clone it. */

		auto* clonedErrorCause = errorCause->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ clonedErrorCause,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->GetStreamIdentifier() == 12345);
		// Reserved bytes must be 0.
		REQUIRE(clonedErrorCause->GetBuffer()[6] == 0);
		REQUIRE(clonedErrorCause->GetBuffer()[7] == 0);

		delete clonedErrorCause;
	}

	SECTION("InvalidStreamIdentifierErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Stream Identifier: 12345
			0x30, 0x39, 0x00, 0x00,
		};
		// clang-format on

		REQUIRE(!InvalidStreamIdentifierErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:1 (INVALID_STREAM_IDENTIFIER), Length: 7
			0x00, 0x01, 0x00, 0x07,
			// Stream Identifier: 12345
			0x30, 0x39, 0x00
		};
		// clang-format on

		REQUIRE(!InvalidStreamIdentifierErrorCause::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Code:1 (INVALID_STREAM_IDENTIFIER), Length: 9
			0x00, 0x01, 0x00, 0x09,
			// Stream Identifier: 12345
			0x30, 0x39, 0x00, 0x00,
			0xEE
		};
		// clang-format on

		REQUIRE(!InvalidStreamIdentifierErrorCause::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Code:1 (INVALID_STREAM_IDENTIFIER), Length: 8
			0x00, 0x01, 0x00, 0x08,
			// Stream Identifier: 12345
			0x30, 0x39, 0x00
		};
		// clang-format on

		REQUIRE(!InvalidStreamIdentifierErrorCause::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("InvalidStreamIdentifierErrorCause::Factory() succeeds")
	{
		auto* errorCause =
		  InvalidStreamIdentifierErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetStreamIdentifier() == 0);
		// Reserved bytes must be 0.
		REQUIRE(errorCause->GetBuffer()[6] == 0);
		REQUIRE(errorCause->GetBuffer()[7] == 0);

		/* Modify it. */

		errorCause->SetStreamIdentifier(6666);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetStreamIdentifier() == 6666);
		// Reserved bytes must be 0.
		REQUIRE(errorCause->GetBuffer()[6] == 0);
		REQUIRE(errorCause->GetBuffer()[7] == 0);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  InvalidStreamIdentifierErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->GetStreamIdentifier() == 6666);
		// Reserved bytes must be 0.
		REQUIRE(parsedErrorCause->GetBuffer()[6] == 0);
		REQUIRE(parsedErrorCause->GetBuffer()[7] == 0);

		delete parsedErrorCause;
	}
}
