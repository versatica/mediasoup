#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/NoUserDataErrorCause.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("No User Data Error Cause (9)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("NoUserDataErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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

		auto* errorCause = RTC::SCTP::NoUserDataErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetTsn() == 987654321);

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetTsn() == 987654321);

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
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->GetTsn() == 987654321);

		delete clonedErrorCause;
	}

	SECTION("NoUserDataErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// TSN: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::NoUserDataErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Code:9 (NO_USER_DATA), Length: 7
			0x00, 0x09, 0x00, 0x07,
			// TSN: 987654321
			0x3A, 0xDE, 0x68,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::NoUserDataErrorCause::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// Code:9 (NO_USER_DATA), Length: 9
			0x00, 0x09, 0x00, 0x09,
			// TSN: 987654321
			0x3A, 0xDE, 0x68, 0xB1,
			0xEE
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::NoUserDataErrorCause::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// Code:9 (NO_USER_DATA), Length: 8
			0x00, 0x09, 0x00, 0x08,
			// TSN (last byte missing)
			0x3A, 0xDE, 0x68
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::NoUserDataErrorCause::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("NoUserDataErrorCause::Factory() succeeds")
	{
		auto* errorCause = RTC::SCTP::NoUserDataErrorCause::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetTsn() == 0);

		/* Modify it. */

		errorCause->SetTsn(666666);

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetTsn() == 666666);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  RTC::SCTP::NoUserDataErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::NO_USER_DATA,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->GetTsn() == 666666);

		delete parsedErrorCause;
	}
}
