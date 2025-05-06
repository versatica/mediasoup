#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Protocol Violation Error Cause (13)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("ProtocolViolationErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:13 (PROTOCOL_VIOLATION), Length: 10
			0x00, 0x0D, 0x00, 0x0A,
			// Additional Information: 0x1234567890AB
			0x12, 0x34, 0x56, 0x78,
			// 2 bytes of padding.
			0x90, 0xAB, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC,
		};
		// clang-format on

		auto* errorCause = ProtocolViolationErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasAdditionalInformation() == true);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 6);
		REQUIRE(errorCause->GetAdditionalInformation()[0] == 0x12);
		REQUIRE(errorCause->GetAdditionalInformation()[1] == 0x34);
		REQUIRE(errorCause->GetAdditionalInformation()[2] == 0x56);
		REQUIRE(errorCause->GetAdditionalInformation()[3] == 0x78);
		REQUIRE(errorCause->GetAdditionalInformation()[4] == 0x90);
		REQUIRE(errorCause->GetAdditionalInformation()[5] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(errorCause->GetAdditionalInformation()[7] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetAdditionalInformation(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasAdditionalInformation() == true);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 6);
		REQUIRE(errorCause->GetAdditionalInformation()[0] == 0x12);
		REQUIRE(errorCause->GetAdditionalInformation()[1] == 0x34);
		REQUIRE(errorCause->GetAdditionalInformation()[2] == 0x56);
		REQUIRE(errorCause->GetAdditionalInformation()[3] == 0x78);
		REQUIRE(errorCause->GetAdditionalInformation()[4] == 0x90);
		REQUIRE(errorCause->GetAdditionalInformation()[5] == 0xAB);
		// These should be padding.
		REQUIRE(errorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(errorCause->GetAdditionalInformation()[7] == 0x00);

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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->HasAdditionalInformation() == true);
		REQUIRE(clonedErrorCause->GetAdditionalInformationLength() == 6);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[0] == 0x12);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[1] == 0x34);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[2] == 0x56);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[3] == 0x78);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[4] == 0x90);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[5] == 0xAB);
		// These should be padding.
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[7] == 0x00);

		delete clonedErrorCause;
	}

	SECTION("ProtocolViolationErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Additional Information: 0x12345678
			0x12, 0x34, 0x56, 0x78,
		};
		// clang-format on

		REQUIRE(!ProtocolViolationErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:13 (PROTOCOL_VIOLATION), Length: 7
			0x00, 0x0D, 0x00, 0x07,
			// Additional Information: 0x123456 (missing padding byte)
			0x12, 0x34, 0x56,
		};
		// clang-format on

		REQUIRE(!ProtocolViolationErrorCause::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("ProtocolViolationErrorCause::Factory() succeeds")
	{
		auto* errorCause = ProtocolViolationErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasAdditionalInformation() == false);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetAdditionalInformation(DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasAdditionalInformation() == true);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 3000);

		errorCause->SetAdditionalInformation(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasAdditionalInformation() == false);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetAdditionalInformation(DataBuffer, 6);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasAdditionalInformation() == true);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 6);
		REQUIRE(errorCause->GetAdditionalInformation()[0] == 0x00);
		REQUIRE(errorCause->GetAdditionalInformation()[1] == 0x01);
		REQUIRE(errorCause->GetAdditionalInformation()[2] == 0x02);
		REQUIRE(errorCause->GetAdditionalInformation()[3] == 0x03);
		REQUIRE(errorCause->GetAdditionalInformation()[4] == 0x04);
		REQUIRE(errorCause->GetAdditionalInformation()[5] == 0x05);
		// These should be padding.
		REQUIRE(errorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(errorCause->GetAdditionalInformation()[7] == 0x00);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  ProtocolViolationErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->HasAdditionalInformation() == true);
		REQUIRE(parsedErrorCause->GetAdditionalInformationLength() == 6);
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[0] == 0x00);
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[1] == 0x01);
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[2] == 0x02);
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[3] == 0x03);
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[4] == 0x04);
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[5] == 0x05);
		// These should be padding.
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[7] == 0x00);

		delete parsedErrorCause;
	}
}
