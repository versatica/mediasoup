#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedParametersErrorCause.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Unrecognized Parameters Error Cause (8)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("UnrecognizedParametersErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Code:8 (UNRECOGNIZED-PARAMETERS), Length: 11
			0x00, 0x08, 0x00, 0x0B,
			// Unrecognized Parameters: 0x1234567890ABCD
			0x12, 0x34, 0x56, 0x78,
			// 1 byte of padding.
			0x90, 0xAB, 0xCD, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::UnrecognizedParametersErrorCause> errorCause{
			RTC::SCTP::UnrecognizedParametersErrorCause::Parse(buffer, sizeof(buffer))
		};

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
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

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
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

		errorCause.reset(errorCause->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
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
	}

	SECTION("UnrecognizedParametersErrorCause::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::UnrecognizedParametersErrorCause> errorCause{
			RTC::SCTP::UnrecognizedParametersErrorCause::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnrecognizedParameters() == false);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetUnrecognizedParameters(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasUnrecognizedParameters() == true);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 3000);

		errorCause->SetUnrecognizedParameters(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasUnrecognizedParameters() == false);
		REQUIRE(errorCause->GetUnrecognizedParametersLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetUnrecognizedParameters(sctpCommon::DataBuffer, 6);

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
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

		errorCause.reset(
		  RTC::SCTP::UnrecognizedParametersErrorCause::Parse(
		    errorCause->GetBuffer(), errorCause->GetLength()));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS,
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
	}
}
