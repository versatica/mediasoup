#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedParametersErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Unrecognized Parameters Error Cause (8)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnrecognizedParametersErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:8 (UNRECOGNIZED_PARAMETERS), Length: 11
			0x00, 0x08, 0x00, 0x0B,
			// Unrecognized Parameters: 0x1234567890ABCD
			0x12, 0x34, 0x56, 0x78,
			// 1 byte of padding.
			0x90, 0xAB, 0xCD, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC,
		};
		// clang-format on

		auto* errorCause = UnrecognizedParametersErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedParameters() == true);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 7);
		REQUIRE(errorCause->GetUnrecognizedParameters()[0] == 0x12);
		REQUIRE(errorCause->GetUnrecognizedParameters()[1] == 0x34);
		REQUIRE(errorCause->GetUnrecognizedParameters()[2] == 0x56);
		REQUIRE(errorCause->GetUnrecognizedParameters()[3] == 0x78);
		REQUIRE(errorCause->GetUnrecognizedParameters()[4] == 0x90);
		REQUIRE(errorCause->GetUnrecognizedParameters()[5] == 0xAB);
		REQUIRE(errorCause->GetUnrecognizedParameters()[6] == 0xCD);
		// This should be padding.
		REQUIRE(errorCause->GetUnrecognizedParameters()[7] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetUnrecognizedParameters(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedParameters() == true);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 7);
		REQUIRE(errorCause->GetUnrecognizedParameters()[0] == 0x12);
		REQUIRE(errorCause->GetUnrecognizedParameters()[1] == 0x34);
		REQUIRE(errorCause->GetUnrecognizedParameters()[2] == 0x56);
		REQUIRE(errorCause->GetUnrecognizedParameters()[3] == 0x78);
		REQUIRE(errorCause->GetUnrecognizedParameters()[4] == 0x90);
		REQUIRE(errorCause->GetUnrecognizedParameters()[5] == 0xAB);
		REQUIRE(errorCause->GetUnrecognizedParameters()[6] == 0xCD);
		// This should be padding.
		REQUIRE(errorCause->GetUnrecognizedParameters()[7] == 0x00);

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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->HasUnrecognizedParameters() == true);
		REQUIRE(clonedErrorCause->GetUnrecognizedParametersLength() == 7);
		REQUIRE(clonedErrorCause->GetUnrecognizedParameters()[0] == 0x12);
		REQUIRE(clonedErrorCause->GetUnrecognizedParameters()[1] == 0x34);
		REQUIRE(clonedErrorCause->GetUnrecognizedParameters()[2] == 0x56);
		REQUIRE(clonedErrorCause->GetUnrecognizedParameters()[3] == 0x78);
		REQUIRE(clonedErrorCause->GetUnrecognizedParameters()[4] == 0x90);
		REQUIRE(clonedErrorCause->GetUnrecognizedParameters()[5] == 0xAB);
		REQUIRE(clonedErrorCause->GetUnrecognizedParameters()[6] == 0xCD);
		// This should be padding.
		REQUIRE(clonedErrorCause->GetUnrecognizedParameters()[7] == 0x00);

		delete clonedErrorCause;
	}

	SECTION("UnrecognizedParametersErrorCause::Factory() succeeds")
	{
		auto* errorCause =
		  UnrecognizedParametersErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedParameters() == false);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetUnrecognizedParameters(DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasUnrecognizedParameters() == true);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 3000);

		errorCause->SetUnrecognizedParameters(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasUnrecognizedParameters() == false);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetUnrecognizedParameters(DataBuffer, 6);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedParameters() == true);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 6);
		REQUIRE(errorCause->GetUnrecognizedParameters()[0] == 0x00);
		REQUIRE(errorCause->GetUnrecognizedParameters()[1] == 0x01);
		REQUIRE(errorCause->GetUnrecognizedParameters()[2] == 0x02);
		REQUIRE(errorCause->GetUnrecognizedParameters()[3] == 0x03);
		REQUIRE(errorCause->GetUnrecognizedParameters()[4] == 0x04);
		REQUIRE(errorCause->GetUnrecognizedParameters()[5] == 0x05);
		// These should be padding.
		REQUIRE(errorCause->GetUnrecognizedParameters()[6] == 0x00);
		REQUIRE(errorCause->GetUnrecognizedParameters()[7] == 0x00);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  UnrecognizedParametersErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->HasUnrecognizedParameters() == true);
		REQUIRE(parsedErrorCause->GetUnrecognizedParametersLength() == 6);
		REQUIRE(parsedErrorCause->GetUnrecognizedParameters()[0] == 0x00);
		REQUIRE(parsedErrorCause->GetUnrecognizedParameters()[1] == 0x01);
		REQUIRE(parsedErrorCause->GetUnrecognizedParameters()[2] == 0x02);
		REQUIRE(parsedErrorCause->GetUnrecognizedParameters()[3] == 0x03);
		REQUIRE(parsedErrorCause->GetUnrecognizedParameters()[4] == 0x04);
		REQUIRE(parsedErrorCause->GetUnrecognizedParameters()[5] == 0x05);
		// These should be padding.
		REQUIRE(parsedErrorCause->GetUnrecognizedParameters()[6] == 0x00);
		REQUIRE(parsedErrorCause->GetUnrecognizedParameters()[7] == 0x00);

		delete parsedErrorCause;
	}
}
