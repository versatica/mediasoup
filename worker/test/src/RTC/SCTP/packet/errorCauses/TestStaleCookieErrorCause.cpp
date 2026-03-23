#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/StaleCookieErrorCause.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Stale Cookie Error Cause (3)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("StaleCookieErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* errorCause = RTC::SCTP::StaleCookieErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetMeasureOfStaleness() == 987654321);

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetMeasureOfStaleness() == 987654321);

		/* Clone it. */

		auto* clonedErrorCause =
		  errorCause->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ clonedErrorCause,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->GetMeasureOfStaleness() == 987654321);

		delete clonedErrorCause;
	}

	SECTION("StaleCookieErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Measure of Staleness: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::StaleCookieErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Code:3 (STALE_COOKIE), Length: 7
			0x00, 0x03, 0x00, 0x07,
			// Measure of Staleness: 987654321
			0x3A, 0xDE, 0x68,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::StaleCookieErrorCause::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// Code:3 (STALE_COOKIE), Length: 9
			0x00, 0x03, 0x00, 0x09,
			// Measure of Staleness: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
			0xEE
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::StaleCookieErrorCause::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// Code:3 (STALE_COOKIE), Length: 8
			0x00, 0x03, 0x00, 0x08,
			// Measure of Staleness (last byte missing)
			0x3A, 0xDE, 0x68
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::StaleCookieErrorCause::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("StaleCookieErrorCause::Factory() succeeds")
	{
		auto* errorCause = RTC::SCTP::StaleCookieErrorCause::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetMeasureOfStaleness() == 0);

		/* Modify it. */

		errorCause->SetMeasureOfStaleness(666666);

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetMeasureOfStaleness() == 666666);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  RTC::SCTP::StaleCookieErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->GetMeasureOfStaleness() == 666666);

		delete parsedErrorCause;
	}
}
