#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UserInitiatedAbortErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("User-Initiated Abort Error Cause (12)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UserInitiatedAbortErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:12 (USER_INITIATED_ABORT), Length: 10
			0x00, 0x0C, 0x00, 0x0A,
			// Upper Layer Abort Reason: 0x1234567890AB
			0x12, 0x34, 0x56, 0x78,
			// 2 bytes of padding.
			0x90, 0xAB, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC,
		};
		// clang-format on

		auto* errorCause = UserInitiatedAbortErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(errorCause->GetUpperLayerAbortReasonLength() == 6);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[0] == 0x12);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[1] == 0x34);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[2] == 0x56);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[3] == 0x78);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[4] == 0x90);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[5] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause->GetUpperLayerAbortReason()[6] == 0x00);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[7] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetUpperLayerAbortReason(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(errorCause->GetUpperLayerAbortReasonLength() == 6);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[0] == 0x12);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[1] == 0x34);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[2] == 0x56);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[3] == 0x78);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[4] == 0x90);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[5] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause->GetUpperLayerAbortReason()[6] == 0x00);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[7] == 0x00);

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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReasonLength() == 6);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason()[0] == 0x12);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason()[1] == 0x34);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason()[2] == 0x56);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason()[3] == 0x78);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason()[4] == 0x90);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason()[5] == 0xAB);
		// These should be padding.
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason()[6] == 0x00);
		REQUIRE(clonedErrorCause->GetUpperLayerAbortReason()[7] == 0x00);

		delete clonedErrorCause;
	}

	SECTION("UserInitiatedAbortErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Upper Layer Abort Reason: 0x12345678
			0x12, 0x34, 0x56, 0x78,
		};
		// clang-format on

		REQUIRE(!UserInitiatedAbortErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:12 (USER_INITIATED_ABORT), Length: 7
			0x00, 0x0C, 0x00, 0x07,
			// Upper Layer Abort Reason: 0x123456 (missing padding byte)
			0x12, 0x34, 0x56,
		};
		// clang-format on

		REQUIRE(!UserInitiatedAbortErrorCause::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("UserInitiatedAbortErrorCause::Factory() succeeds")
	{
		auto* errorCause = UserInitiatedAbortErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUpperLayerAbortReason() == false);
		REQUIRE(errorCause->GetUpperLayerAbortReasonLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetUpperLayerAbortReason(DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(errorCause->GetUpperLayerAbortReasonLength() == 3000);

		errorCause->SetUpperLayerAbortReason(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasUpperLayerAbortReason() == false);
		REQUIRE(errorCause->GetUpperLayerAbortReasonLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetUpperLayerAbortReason(DataBuffer, 6);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(errorCause->GetUpperLayerAbortReasonLength() == 6);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[0] == 0x00);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[1] == 0x01);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[2] == 0x02);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[3] == 0x03);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[4] == 0x04);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[5] == 0x05);
		// These should be padding.
		REQUIRE(errorCause->GetUpperLayerAbortReason()[6] == 0x00);
		REQUIRE(errorCause->GetUpperLayerAbortReason()[7] == 0x00);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  UserInitiatedAbortErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->HasUpperLayerAbortReason() == true);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReasonLength() == 6);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason()[0] == 0x00);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason()[1] == 0x01);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason()[2] == 0x02);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason()[3] == 0x03);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason()[4] == 0x04);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason()[5] == 0x05);
		// These should be padding.
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason()[6] == 0x00);
		REQUIRE(parsedErrorCause->GetUpperLayerAbortReason()[7] == 0x00);

		delete parsedErrorCause;
	}
}
