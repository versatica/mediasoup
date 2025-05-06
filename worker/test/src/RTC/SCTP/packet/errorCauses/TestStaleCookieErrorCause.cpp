#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/StaleCookieErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Stale Cookie Error Cause (3)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("StaleCookieErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:3 (STALE_COOKIE), Length: 8
			0x00, 0x03, 0x00, 0x08,
			// Measure of Staleness: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* errorCause = StaleCookieErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetMeasureOfStaleness() == 987654321);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetMeasureOfStaleness(44444), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetMeasureOfStaleness() == 987654321);

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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->GetMeasureOfStaleness() == 987654321);

		delete clonedErrorCause;
	}

	SECTION("StaleCookieErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Measure of Staleness: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
		};
		// clang-format on

		REQUIRE(!StaleCookieErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:3 (STALE_COOKIE), Length: 7
			0x00, 0x03, 0x00, 0x07,
			// Measure of Staleness: 987654321
			0x3A, 0xDE, 0x68,
		};
		// clang-format on

		REQUIRE(!StaleCookieErrorCause::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Code:3 (STALE_COOKIE), Length: 9
			0x00, 0x03, 0x00, 0x09,
			// Measure of Staleness: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
			0xEE
		};
		// clang-format on

		REQUIRE(!StaleCookieErrorCause::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Code:3 (STALE_COOKIE), Length: 8
			0x00, 0x03, 0x00, 0x08,
			// Measure of Staleness (last byte missing)
			0x3A, 0xDE, 0x68
		};
		// clang-format on

		REQUIRE(!StaleCookieErrorCause::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("StaleCookieErrorCause::Factory() succeeds")
	{
		auto* errorCause = StaleCookieErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetMeasureOfStaleness() == 0);

		/* Modify it. */

		errorCause->SetMeasureOfStaleness(666666);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetMeasureOfStaleness() == 666666);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  StaleCookieErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->GetMeasureOfStaleness() == 666666);

		delete parsedErrorCause;
	}
}
