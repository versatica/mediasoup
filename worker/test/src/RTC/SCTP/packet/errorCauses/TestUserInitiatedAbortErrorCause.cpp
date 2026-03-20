#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UserInitiatedAbortErrorCause.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("User-Initiated Abort Error Cause (12)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("UserInitiatedAbortErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Code:12 (USER_INITIATED_ABORT), Length: 10
			0x00, 0x0C, 0x00, 0x0A,
			// Upper Layer Abort Reason: "I DIE!"
			0x49, 0x20, 0x44, 0x49,
			// 2 bytes of padding.
			0x45, 0x21, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC,
		};
		// clang-format on

		auto* errorCause = RTC::SCTP::UserInitiatedAbortErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(errorCause->GetUpperLayerAbortReason() == "I DIE!");

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(errorCause->GetUpperLayerAbortReason() == "I DIE!");

		/* Clone it. */

		auto* clonedErrorCause =
		  errorCause->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ clonedErrorCause,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason() == "I DIE!");

		delete clonedErrorCause;
	}

	SECTION("UserInitiatedAbortErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Upper Layer Abort Reason: 0x12345678
			0x12, 0x34, 0x56, 0x78,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::UserInitiatedAbortErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Code:12 (USER_INITIATED_ABORT), Length: 7
			0x00, 0x0C, 0x00, 0x07,
			// Upper Layer Abort Reason: 0x123456 (missing padding byte)
			0x12, 0x34, 0x56,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::UserInitiatedAbortErrorCause::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("UserInitiatedAbortErrorCause::Factory() succeeds")
	{
		auto* errorCause = RTC::SCTP::UserInitiatedAbortErrorCause::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUpperLayerAbortReason() == false);
		REQUIRE(errorCause->GetUpperLayerAbortReason().empty());

		/* Modify it. */

		// Verify that replacing the value works. This is 17 bytes long.
		errorCause->SetUpperLayerAbortReason("I'm dying! ☺️");

		REQUIRE(errorCause->GetLength() == 24);
		REQUIRE(errorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(errorCause->GetUpperLayerAbortReason() == "I'm dying! ☺️");

		errorCause->SetUpperLayerAbortReason("");

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasUpperLayerAbortReason() == false);
		REQUIRE(errorCause->GetUpperLayerAbortReason().empty());

		// 6 bytes + 2 bytes of padding.
		errorCause->SetUpperLayerAbortReason("go go go");

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(errorCause->GetUpperLayerAbortReason() == "go go go");

		/* Parse itself and compare. */

		auto* parsedErrorCause = RTC::SCTP::UserInitiatedAbortErrorCause::Parse(
		  errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason() == "go go go");

		delete parsedErrorCause;
	}
}
