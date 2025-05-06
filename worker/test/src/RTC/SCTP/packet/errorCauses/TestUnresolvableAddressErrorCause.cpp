#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnresolvableAddressErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Unresolvable Address Error Cause (5)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("UnresolvableAddressErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:5 (UNRESOLVABLE_ADDRESS), Length: 9
			0x00, 0x05, 0x00, 0x09,
			// Unresolvable Address: 0x1234567890
			0x12, 0x34, 0x56, 0x78,
			// 3 bytes of padding.
			0x90, 0x00, 0x00, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC,
		};
		// clang-format on

		auto* errorCause = UnresolvableAddressErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnresolvableAddress() == true);
		REQUIRE(errorCause->GetUnresolvableAddressLength() == 5);
		REQUIRE(errorCause->GetUnresolvableAddress()[0] == 0x12);
		REQUIRE(errorCause->GetUnresolvableAddress()[1] == 0x34);
		REQUIRE(errorCause->GetUnresolvableAddress()[2] == 0x56);
		REQUIRE(errorCause->GetUnresolvableAddress()[3] == 0x78);
		REQUIRE(errorCause->GetUnresolvableAddress()[4] == 0x90);
		// These should be padding.
		REQUIRE(errorCause->GetUnresolvableAddress()[5] == 0x00);
		REQUIRE(errorCause->GetUnresolvableAddress()[6] == 0x00);
		REQUIRE(errorCause->GetUnresolvableAddress()[7] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetUnresolvableAddress(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnresolvableAddress() == true);
		REQUIRE(errorCause->GetUnresolvableAddressLength() == 5);
		REQUIRE(errorCause->GetUnresolvableAddress()[0] == 0x12);
		REQUIRE(errorCause->GetUnresolvableAddress()[1] == 0x34);
		REQUIRE(errorCause->GetUnresolvableAddress()[2] == 0x56);
		REQUIRE(errorCause->GetUnresolvableAddress()[3] == 0x78);
		REQUIRE(errorCause->GetUnresolvableAddress()[4] == 0x90);
		// These should be padding.
		REQUIRE(errorCause->GetUnresolvableAddress()[5] == 0x00);
		REQUIRE(errorCause->GetUnresolvableAddress()[6] == 0x00);
		REQUIRE(errorCause->GetUnresolvableAddress()[7] == 0x00);

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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->HasUnresolvableAddress() == true);
		REQUIRE(clonedErrorCause->GetUnresolvableAddressLength() == 5);
		REQUIRE(clonedErrorCause->GetUnresolvableAddress()[0] == 0x12);
		REQUIRE(clonedErrorCause->GetUnresolvableAddress()[1] == 0x34);
		REQUIRE(clonedErrorCause->GetUnresolvableAddress()[2] == 0x56);
		REQUIRE(clonedErrorCause->GetUnresolvableAddress()[3] == 0x78);
		REQUIRE(clonedErrorCause->GetUnresolvableAddress()[4] == 0x90);
		// These should be padding.
		REQUIRE(clonedErrorCause->GetUnresolvableAddress()[5] == 0x00);
		REQUIRE(clonedErrorCause->GetUnresolvableAddress()[6] == 0x00);
		REQUIRE(clonedErrorCause->GetUnresolvableAddress()[7] == 0x00);

		delete clonedErrorCause;
	}

	SECTION("UnresolvableAddressErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			// Unresolvable Address: 0x12345678
			0x12, 0x34, 0x56, 0x78,
		};
		// clang-format on

		REQUIRE(!UnresolvableAddressErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:5 (UNRESOLVABLE_ADDRESS), Length: 7
			0x00, 0x05, 0x00, 0x07,
			// Unresolvable Address: 0x123456 (missing padding byte)
			0x12, 0x34, 0x56,
		};
		// clang-format on

		REQUIRE(!UnresolvableAddressErrorCause::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("UnresolvableAddressErrorCause::Factory() succeeds")
	{
		auto* errorCause = UnresolvableAddressErrorCause::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnresolvableAddress() == false);
		REQUIRE(errorCause->GetUnresolvableAddressLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetUnresolvableAddress(DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasUnresolvableAddress() == true);
		REQUIRE(errorCause->GetUnresolvableAddressLength() == 3000);

		errorCause->SetUnresolvableAddress(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasUnresolvableAddress() == false);
		REQUIRE(errorCause->GetUnresolvableAddressLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetUnresolvableAddress(DataBuffer, 6);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasUnresolvableAddress() == true);
		REQUIRE(errorCause->GetUnresolvableAddressLength() == 6);
		REQUIRE(errorCause->GetUnresolvableAddress()[0] == 0x00);
		REQUIRE(errorCause->GetUnresolvableAddress()[1] == 0x01);
		REQUIRE(errorCause->GetUnresolvableAddress()[2] == 0x02);
		REQUIRE(errorCause->GetUnresolvableAddress()[3] == 0x03);
		REQUIRE(errorCause->GetUnresolvableAddress()[4] == 0x04);
		REQUIRE(errorCause->GetUnresolvableAddress()[5] == 0x05);
		// These should be padding.
		REQUIRE(errorCause->GetUnresolvableAddress()[6] == 0x00);
		REQUIRE(errorCause->GetUnresolvableAddress()[7] == 0x00);

		/* Parse itself and compare. */

		auto* parsedErrorCause =
		  UnresolvableAddressErrorCause::Parse(errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->HasUnresolvableAddress() == true);
		REQUIRE(parsedErrorCause->GetUnresolvableAddressLength() == 6);
		REQUIRE(parsedErrorCause->GetUnresolvableAddress()[0] == 0x00);
		REQUIRE(parsedErrorCause->GetUnresolvableAddress()[1] == 0x01);
		REQUIRE(parsedErrorCause->GetUnresolvableAddress()[2] == 0x02);
		REQUIRE(parsedErrorCause->GetUnresolvableAddress()[3] == 0x03);
		REQUIRE(parsedErrorCause->GetUnresolvableAddress()[4] == 0x04);
		REQUIRE(parsedErrorCause->GetUnresolvableAddress()[5] == 0x05);
		// These should be padding.
		REQUIRE(parsedErrorCause->GetUnresolvableAddress()[6] == 0x00);
		REQUIRE(parsedErrorCause->GetUnresolvableAddress()[7] == 0x00);

		delete parsedErrorCause;
	}
}
