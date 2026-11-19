#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/OutOfResourceErrorCause.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Out of Resource Error Cause (4)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("OutOfResourceErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Code:4 (OUT-OF-RESOURCE), Length: 4
			0x00, 0x04, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::OutOfResourceErrorCause> errorCause{
			RTC::SCTP::OutOfResourceErrorCause::Parse(buffer, sizeof(buffer))
		};

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		/* Clone it. */

		errorCause.reset(errorCause->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);
	}

	SECTION("OutOfResourceErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 4
			0x03, 0xE7, 0x00, 0x04
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::OutOfResourceErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong length field.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Code:4 (OUT-OF-RESOURCE), Length: 5
			0x00, 0x04, 0x00, 0x05,
			// 0x3A, 3 bytes of padding
			0x3A, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::OutOfResourceErrorCause::Parse(buffer2, sizeof(buffer2)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// Code:4 (OUT-OF-RESOURCE), length (broken)
			0x00, 0x04, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::OutOfResourceErrorCause::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("OutOfResourceErrorCause::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::OutOfResourceErrorCause> errorCause{
			RTC::SCTP::OutOfResourceErrorCause::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		/* Parse itself and compare. */

		errorCause.reset(
		  RTC::SCTP::OutOfResourceErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength()));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);
	}
}
