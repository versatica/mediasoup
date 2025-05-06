#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/NoUserDataErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("No User Data Error Cause (9)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("NoUserDataErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:9 (NO_USER_DATA), Length: 8
			0x00, 0x09, 0x00, 0x08,
			// TSN: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* errorCause = NoUserDataErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetTsn() == 987654321);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetTsn(44444), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetTsn() == 987654321);

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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->GetTsn() == 987654321);

		delete clonedErrorCause;
	}

	SECTION("NoUserDataErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// TSN: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
		};
		// clang-format on

		REQUIRE(!NoUserDataErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:9 (NO_USER_DATA), Length: 7
			0x00, 0x09, 0x00, 0x07,
			// TSN: 987654321
			0x3A, 0xDE, 0x68,
		};
		// clang-format on

		REQUIRE(!NoUserDataErrorCause::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Code:9 (NO_USER_DATA), Length: 9
			0x00, 0x09, 0x00, 0x09,
			// TSN: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
			0xEE
		};
		// clang-format on

		REQUIRE(!NoUserDataErrorCause::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Code:9 (NO_USER_DATA), Length: 8
			0x00, 0x09, 0x00, 0x08,
			// TSN (last byte missing)
			0x3A, 0xDE, 0x68
		};
		// clang-format on

		REQUIRE(!NoUserDataErrorCause::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("NoUserDataErrorCause::Factory() succeeds")
	{
		auto* errorCause = NoUserDataErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetTsn() == 0);

		/* Modify it. */

		errorCause->SetTsn(666666);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetTsn() == 666666);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  NoUserDataErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->GetTsn() == 666666);

		delete parsedErrorCause;
	}
}
