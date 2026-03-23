#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Protocol Violation Error Cause (13)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("ProtocolViolationErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Code:13 (PROTOCOL_VIOLATION), Length: 10
			0x00, 0x0D, 0x00, 0x0A,
			// Additional Information: "error1"
			0x65, 0x72, 0x72, 0x6F,
			// 2 bytes of padding.
			0x72, 0x31, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC,
		};
		// clang-format on

		auto* errorCause = RTC::SCTP::ProtocolViolationErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasAdditionalInformation() == true);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 6);
		REQUIRE(errorCause->GetAdditionalInformation()[0] == 0x65);
		REQUIRE(errorCause->GetAdditionalInformation()[1] == 0x72);
		REQUIRE(errorCause->GetAdditionalInformation()[2] == 0x72);
		REQUIRE(errorCause->GetAdditionalInformation()[3] == 0x6F);
		REQUIRE(errorCause->GetAdditionalInformation()[4] == 0x72);
		REQUIRE(errorCause->GetAdditionalInformation()[5] == 0x31);

		std::string additionalInfo(
		  reinterpret_cast<const char*>(errorCause->GetAdditionalInformation()),
		  errorCause->GetAdditionalInformationLength());

		REQUIRE(additionalInfo == "error1");
		// These should be padding.
		REQUIRE(errorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(errorCause->GetAdditionalInformation()[7] == 0x00);

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasAdditionalInformation() == true);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 6);
		REQUIRE(errorCause->GetAdditionalInformation()[0] == 0x65);
		REQUIRE(errorCause->GetAdditionalInformation()[1] == 0x72);
		REQUIRE(errorCause->GetAdditionalInformation()[2] == 0x72);
		REQUIRE(errorCause->GetAdditionalInformation()[3] == 0x6F);
		REQUIRE(errorCause->GetAdditionalInformation()[4] == 0x72);
		REQUIRE(errorCause->GetAdditionalInformation()[5] == 0x31);

		additionalInfo = std::string(
		  reinterpret_cast<const char*>(errorCause->GetAdditionalInformation()),
		  errorCause->GetAdditionalInformationLength());

		REQUIRE(additionalInfo == "error1");
		// These should be padding.
		REQUIRE(errorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(errorCause->GetAdditionalInformation()[7] == 0x00);

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
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->HasAdditionalInformation() == true);
		REQUIRE(clonedErrorCause->GetAdditionalInformationLength() == 6);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[0] == 0x65);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[1] == 0x72);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[2] == 0x72);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[3] == 0x6F);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[4] == 0x72);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[5] == 0x31);

		additionalInfo = std::string(
		  reinterpret_cast<const char*>(clonedErrorCause->GetAdditionalInformation()),
		  clonedErrorCause->GetAdditionalInformationLength());

		REQUIRE(additionalInfo == "error1");
		// These should be padding.
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(clonedErrorCause->GetAdditionalInformation()[7] == 0x00);

		delete clonedErrorCause;
	}

	SECTION("ProtocolViolationErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Additional Information: 0x12345678
			0x12, 0x34, 0x56, 0x78,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::ProtocolViolationErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Code:13 (PROTOCOL_VIOLATION), Length: 7
			0x00, 0x0D, 0x00, 0x07,
			// Additional Information: 0x123456 (missing padding byte)
			0x12, 0x34, 0x56,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::ProtocolViolationErrorCause::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("ProtocolViolationErrorCause::Factory() succeeds")
	{
		auto* errorCause = RTC::SCTP::ProtocolViolationErrorCause::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasAdditionalInformation() == false);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetAdditionalInformation(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasAdditionalInformation() == true);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 3000);

		errorCause->SetAdditionalInformation(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasAdditionalInformation() == false);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetAdditionalInformation("iñaki");

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasAdditionalInformation() == true);
		REQUIRE(errorCause->GetAdditionalInformationLength() == 6);

		std::string additionalInfo(
		  reinterpret_cast<const char*>(errorCause->GetAdditionalInformation()),
		  errorCause->GetAdditionalInformationLength());

		REQUIRE(additionalInfo == "iñaki");
		// These should be padding.
		REQUIRE(errorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(errorCause->GetAdditionalInformation()[7] == 0x00);

		/* Parse itself and compare. */

		auto* parsedErrorCause = RTC::SCTP::ProtocolViolationErrorCause::Parse(
		  errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->HasAdditionalInformation() == true);
		REQUIRE(parsedErrorCause->GetAdditionalInformationLength() == 6);

		additionalInfo = std::string(
		  reinterpret_cast<const char*>(parsedErrorCause->GetAdditionalInformation()),
		  parsedErrorCause->GetAdditionalInformationLength());

		REQUIRE(additionalInfo == "iñaki");

		// These should be padding.
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[6] == 0x00);
		REQUIRE(parsedErrorCause->GetAdditionalInformation()[7] == 0x00);

		delete parsedErrorCause;
	}
}
