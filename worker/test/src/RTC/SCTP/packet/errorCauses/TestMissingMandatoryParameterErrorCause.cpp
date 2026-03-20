#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/errorCauses/MissingMandatoryParameterErrorCause.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Invalid Stream Identifier Error Cause (2)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("MissingMandatoryParameterErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Code:2 (MISSING_MANDATORY_PARAMETER), Length: 14
			0x00, 0x02, 0x00, 0x0E,
			// Number of missing params: 3
			0x00, 0x00, 0x00, 0x03,
			// Missing Param 1: 5 (IPV4_ADDRESS), Missing Param 2: 6 (IPV6_ADDRESS)
			0x00, 0x05, 0x00, 0x06,
			// Missing Param 3: 9 (COOKIE_PRESERVATIVE), 2 bytes of padding
			0x00, 0x09, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* errorCause = RTC::SCTP::MissingMandatoryParameterErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetNumberOfMissingParameters() == 3);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(0) == RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(1) == RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(2) ==
		  RTC::SCTP::Parameter::ParameterType::COOKIE_PRESERVATIVE);
		// These should be padding.
		REQUIRE(errorCause->GetBuffer()[14] == 0);
		REQUIRE(errorCause->GetBuffer()[15] == 0);

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 16,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetNumberOfMissingParameters() == 3);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(0) == RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(1) == RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(2) ==
		  RTC::SCTP::Parameter::ParameterType::COOKIE_PRESERVATIVE);
		// These should be padding.
		REQUIRE(errorCause->GetBuffer()[14] == 0);
		REQUIRE(errorCause->GetBuffer()[15] == 0);

		// /* Clone it. */

		auto* clonedErrorCause =
		  errorCause->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ clonedErrorCause,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 16,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->GetNumberOfMissingParameters() == 3);
		REQUIRE(
		  clonedErrorCause->GetMissingParameterTypeAt(0) ==
		  RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS);
		REQUIRE(
		  clonedErrorCause->GetMissingParameterTypeAt(1) ==
		  RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS);
		REQUIRE(
		  clonedErrorCause->GetMissingParameterTypeAt(2) ==
		  RTC::SCTP::Parameter::ParameterType::COOKIE_PRESERVATIVE);
		// These should be padding.
		REQUIRE(clonedErrorCause->GetBuffer()[14] == 0);
		REQUIRE(clonedErrorCause->GetBuffer()[15] == 0);

		delete clonedErrorCause;
	}

	SECTION("MissingMandatoryParameterErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 14
			0x03, 0xE7, 0x00, 0x0E,
			// Number of missing params: 3
			0x00, 0x00, 0x00, 0x03,
			// Missing Param 1: 5 (IPV4_ADDRESS), Missing Param 2: 6 (IPV6_ADDRESS)
			0x00, 0x05, 0x00, 0x06,
			// Missing Param 3: 9 (COOKIE_PRESERVATIVE), 2 bytes of padding
			0x00, 0x09, 0x00, 0x00,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::MissingMandatoryParameterErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Length field doesn't match Number of missing parameters.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Code:2 (MISSING_MANDATORY_PARAMETER), Length: 14
			0x00, 0x02, 0x00, 0x0E,
			// Number of missing params: 2
			0x00, 0x00, 0x00, 0x02,
			// Missing Param 1: 5 (IPV4_ADDRESS), Missing Param 2: 6 (IPV6_ADDRESS)
			0x00, 0x05, 0x00, 0x06,
			// Missing Param 3: 9 (COOKIE_PRESERVATIVE) (exceeds number of missing
			// parameters), 2 bytes of padding
			0x00, 0x09, 0x00, 0x00,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::MissingMandatoryParameterErrorCause::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field (smaller than buffer).
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// Code:2 (MISSING_MANDATORY_PARAMETER), Length: 8 (buffer is 12)
			0x00, 0x02, 0x00, 0x08,
			// Number of missing params: 4
			0x00, 0x00, 0x00, 0x02,
			// Missing Param 1: 5 (IPV4_ADDRESS), Missing Param 2: 6 (IPV6_ADDRESS)
			0x00, 0x05, 0x00, 0x06,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::MissingMandatoryParameterErrorCause::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("MissingMandatoryParameterErrorCause::Factory() succeeds")
	{
		auto* errorCause = RTC::SCTP::MissingMandatoryParameterErrorCause::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetNumberOfMissingParameters() == 0);

		/* Modify it. */

		errorCause->AddMissingParameterType(RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS);
		errorCause->AddMissingParameterType(RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS);
		errorCause->AddMissingParameterType(RTC::SCTP::Parameter::ParameterType::COOKIE_PRESERVATIVE);

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 16,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->GetNumberOfMissingParameters() == 3);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(0) == RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(1) == RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS);
		REQUIRE(
		  errorCause->GetMissingParameterTypeAt(2) ==
		  RTC::SCTP::Parameter::ParameterType::COOKIE_PRESERVATIVE);
		// These should be padding.
		REQUIRE(errorCause->GetBuffer()[14] == 0);
		REQUIRE(errorCause->GetBuffer()[15] == 0);

		/* Parse itself and compare. */

		auto* parsedErrorCause = RTC::SCTP::MissingMandatoryParameterErrorCause::Parse(
		  errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 16,
		  /*length*/ 16,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->GetNumberOfMissingParameters() == 3);
		REQUIRE(
		  parsedErrorCause->GetMissingParameterTypeAt(0) ==
		  RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS);
		REQUIRE(
		  parsedErrorCause->GetMissingParameterTypeAt(1) ==
		  RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS);
		REQUIRE(
		  parsedErrorCause->GetMissingParameterTypeAt(2) ==
		  RTC::SCTP::Parameter::ParameterType::COOKIE_PRESERVATIVE);
		// These should be padding.
		REQUIRE(parsedErrorCause->GetBuffer()[14] == 0);
		REQUIRE(parsedErrorCause->GetBuffer()[15] == 0);

		delete parsedErrorCause;
	}
}
