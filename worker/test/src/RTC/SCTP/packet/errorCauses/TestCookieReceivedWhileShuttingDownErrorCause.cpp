#include "common.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/CookieReceivedWhileShuttingDownErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Cookie Received While Shutting Down Error Cause (10)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("CookieReceivedWhileShuttingDownErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:10 (COOKIE_RECEIVED_WHILE_SHUTTING_DOWN), Length: 4
			0x00, 0x0A, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* errorCause = CookieReceivedWhileShuttingDownErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		/* Clone it. */

		auto* clonedErrorCause = errorCause->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ clonedErrorCause,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		delete clonedErrorCause;
	}

	SECTION("CookieReceivedWhileShuttingDownErrorCause::Factory() succeeds")
	{
		auto* errorCause =
		  CookieReceivedWhileShuttingDownErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		/* Parse itself and compare. */

		auto* parsedErrorCause = CookieReceivedWhileShuttingDownErrorCause::Parse(
		  errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN,
		  /*unknownCode*/ false);

		delete parsedErrorCause;
	}
}
