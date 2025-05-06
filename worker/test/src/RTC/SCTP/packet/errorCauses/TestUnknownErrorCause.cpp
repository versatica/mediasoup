#include "common.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnknownErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Unknown Error Cause", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnknownErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:999 (UNKNOWN), Length: 11
			0x03, 0xE7, 0x00, 0x0B,
			// Unknown value: 0x0123456789ABCD
			0x01, 0x23, 0x45, 0x67,
			// 1 byte of padding
			0x89, 0xAB, 0xCD, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xEE
		};
		// clang-format on

		auto* errorCause = UnknownErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ static_cast<ErrorCause::ErrorCauseCode>(999),
		  /*unknownCode*/ true);

		REQUIRE(errorCause->HasUnknownValue() == true);
		REQUIRE(errorCause->GetUnknownValueLength() == 7);
		REQUIRE(errorCause->GetUnknownValue()[1] == 0x23);
		REQUIRE(errorCause->GetUnknownValue()[0] == 0x01);
		REQUIRE(errorCause->GetUnknownValue()[1] == 0x23);
		REQUIRE(errorCause->GetUnknownValue()[2] == 0x45);
		REQUIRE(errorCause->GetUnknownValue()[3] == 0x67);
		REQUIRE(errorCause->GetUnknownValue()[4] == 0x89);
		REQUIRE(errorCause->GetUnknownValue()[5] == 0xAB);
		REQUIRE(errorCause->GetUnknownValue()[6] == 0xCD);
		// This should be padding.
		REQUIRE(errorCause->GetUnknownValue()[7] == 0x00);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ static_cast<ErrorCause::ErrorCauseCode>(999),
		  /*unknownCode*/ true);

		REQUIRE(errorCause->HasUnknownValue() == true);
		REQUIRE(errorCause->GetUnknownValueLength() == 7);
		REQUIRE(errorCause->GetUnknownValue()[1] == 0x23);
		REQUIRE(errorCause->GetUnknownValue()[0] == 0x01);
		REQUIRE(errorCause->GetUnknownValue()[1] == 0x23);
		REQUIRE(errorCause->GetUnknownValue()[2] == 0x45);
		REQUIRE(errorCause->GetUnknownValue()[3] == 0x67);
		REQUIRE(errorCause->GetUnknownValue()[4] == 0x89);
		REQUIRE(errorCause->GetUnknownValue()[5] == 0xAB);
		REQUIRE(errorCause->GetUnknownValue()[6] == 0xCD);
		// This should be padding.
		REQUIRE(errorCause->GetUnknownValue()[7] == 0x00);

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
		  /*causeCode*/ static_cast<ErrorCause::ErrorCauseCode>(999),
		  /*unknownCode*/ true);

		REQUIRE(clonedErrorCause->HasUnknownValue() == true);
		REQUIRE(clonedErrorCause->GetUnknownValueLength() == 7);
		REQUIRE(clonedErrorCause->GetUnknownValue()[1] == 0x23);
		REQUIRE(clonedErrorCause->GetUnknownValue()[0] == 0x01);
		REQUIRE(clonedErrorCause->GetUnknownValue()[1] == 0x23);
		REQUIRE(clonedErrorCause->GetUnknownValue()[2] == 0x45);
		REQUIRE(clonedErrorCause->GetUnknownValue()[3] == 0x67);
		REQUIRE(clonedErrorCause->GetUnknownValue()[4] == 0x89);
		REQUIRE(clonedErrorCause->GetUnknownValue()[5] == 0xAB);
		REQUIRE(clonedErrorCause->GetUnknownValue()[6] == 0xCD);
		// This should be padding.
		REQUIRE(clonedErrorCause->GetUnknownValue()[7] == 0x00);

		delete clonedErrorCause;
	}

	SECTION("UnknownErrorCause::Parse() fails")
	{
		// Wrong Length field.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:49159 (UNKNOWN), Length: 3
			0xC0, 0x07, 0x00, 0x03,
			// Unknown value: 0x0123456789ABCD
			0x01, 0x23, 0x45, 0x67,
			// 1 byte of padding
			0x89, 0xAB, 0xCD, 0x00,
		};
		// clang-format on

		REQUIRE(!UnknownErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:49159 (UNKNOWN), Length: 11
			0xC0, 0x07, 0x00, 0x0B,
			// Unknown value: 0x0123456789ABCD
			0x01, 0x23, 0x45, 0x67,
			// 1 byte of padding (missing)
			0x89, 0xAB, 0xCD
		};
		// clang-format on

		REQUIRE(!UnknownErrorCause::Parse(buffer2, sizeof(buffer2)));
	}
}
