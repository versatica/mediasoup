#include "common.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/OutOfResourceErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Out of Resource Error Cause (4)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("OutOfResourceErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:4 (OUT_OF_RESOURCE), Length: 4
			0x00, 0x04, 0x00, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* errorCause = OutOfResourceErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		delete clonedErrorCause;
	}

	SECTION("OutOfResourceErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 4
			0x03, 0xE7, 0x00, 0x04
		};
		// clang-format on

		REQUIRE(!OutOfResourceErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:4 (OUT_OF_RESOURCE), Length: 5
			0x00, 0x04, 0x00, 0x07,
			0x3A,
		};
		// clang-format on

		REQUIRE(!OutOfResourceErrorCause::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Code:4 (OUT_OF_RESOURCE), Length (broken)
			0x00, 0x04, 0x00,
		};
		// clang-format on

		REQUIRE(!OutOfResourceErrorCause::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("OutOfResourceErrorCause::Factory() succeeds")
	{
		auto* errorCause = OutOfResourceErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  OutOfResourceErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 4,
		  /*length*/ 4,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE,
		  /*unknownCode*/ false);

		delete parsedErrorCause;
	}
}
