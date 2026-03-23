#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/CookieReceivedWhileShuttingDownErrorCause.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Cookie Received While Shutting Down Error Cause (10)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("CookieReceivedWhileShuttingDownErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Code:10 (COOKIE_RECEIVED_WHILE_SHUTTING_DOWN), Length: 4
			0x00, 0x0A, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* errorCause =
		  RTC::SCTP::CookieReceivedWhileShuttingDownErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		/* Clone it. */

		auto* clonedErrorCause =
		  errorCause->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ clonedErrorCause,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		delete clonedErrorCause;
	}

	SECTION("CookieReceivedWhileShuttingDownErrorCause::Factory() succeeds")
	{
		auto* errorCause = RTC::SCTP::CookieReceivedWhileShuttingDownErrorCause::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		/* Parse itself and compare. */

		auto* parsedErrorCause = RTC::SCTP::CookieReceivedWhileShuttingDownErrorCause::Parse(
		  errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		delete parsedErrorCause;
	}
}
