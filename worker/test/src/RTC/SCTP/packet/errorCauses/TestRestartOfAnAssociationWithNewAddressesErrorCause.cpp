#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/RestartOfAnAssociationWithNewAddressesErrorCause.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Restart of an Association with New Addresses Error Cause (11)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("RestartOfAnAssociationWithNewAddressesErrorCause::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Code:11 (RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES), Length: 11
			0x00, 0x0B, 0x00, 0x0B,
			// New Address TLVs: 0x1234567890AB
			0x12, 0x34, 0x56, 0x78,
			// 1 byte of padding.
			0x90, 0xAB, 0xCD, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC,
		};
		// clang-format on

		auto* errorCause =
		  RestartOfAnAssociationWithNewAddressesErrorCause::Parse(buffer, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasNewAddressTlvs() == true);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 7);
		REQUIRE(errorCause->GetNewAddressTlvs()[0] == 0x12);
		REQUIRE(errorCause->GetNewAddressTlvs()[1] == 0x34);
		REQUIRE(errorCause->GetNewAddressTlvs()[2] == 0x56);
		REQUIRE(errorCause->GetNewAddressTlvs()[3] == 0x78);
		REQUIRE(errorCause->GetNewAddressTlvs()[4] == 0x90);
		REQUIRE(errorCause->GetNewAddressTlvs()[5] == 0xAB);
		REQUIRE(errorCause->GetNewAddressTlvs()[6] == 0xCD);
		// This should be padding.
		REQUIRE(errorCause->GetNewAddressTlvs()[7] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(errorCause->SetNewAddressTlvs(DataBuffer, 3), MediaSoupError);

		/* Serialize it. */

		errorCause->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasNewAddressTlvs() == true);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 7);
		REQUIRE(errorCause->GetNewAddressTlvs()[0] == 0x12);
		REQUIRE(errorCause->GetNewAddressTlvs()[1] == 0x34);
		REQUIRE(errorCause->GetNewAddressTlvs()[2] == 0x56);
		REQUIRE(errorCause->GetNewAddressTlvs()[3] == 0x78);
		REQUIRE(errorCause->GetNewAddressTlvs()[4] == 0x90);
		REQUIRE(errorCause->GetNewAddressTlvs()[5] == 0xAB);
		REQUIRE(errorCause->GetNewAddressTlvs()[6] == 0xCD);
		// This should be padding.
		REQUIRE(errorCause->GetNewAddressTlvs()[7] == 0x00);

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
		  /*causeCode*/ ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
		  /*unknownCode*/ false);

		REQUIRE(clonedErrorCause->HasNewAddressTlvs() == true);
		REQUIRE(clonedErrorCause->GetNewAddressTlvsLength() == 7);
		REQUIRE(clonedErrorCause->GetNewAddressTlvs()[0] == 0x12);
		REQUIRE(clonedErrorCause->GetNewAddressTlvs()[1] == 0x34);
		REQUIRE(clonedErrorCause->GetNewAddressTlvs()[2] == 0x56);
		REQUIRE(clonedErrorCause->GetNewAddressTlvs()[3] == 0x78);
		REQUIRE(clonedErrorCause->GetNewAddressTlvs()[4] == 0x90);
		REQUIRE(clonedErrorCause->GetNewAddressTlvs()[5] == 0xAB);
		REQUIRE(clonedErrorCause->GetNewAddressTlvs()[6] == 0xCD);
		// This should be padding.
		REQUIRE(clonedErrorCause->GetNewAddressTlvs()[7] == 0x00);

		delete clonedErrorCause;
	}

	SECTION("RestartOfAnAssociationWithNewAddressesErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			//  NewAddressTlvs: 0x12345678
			0x12, 0x34, 0x56, 0x78,
		};
		// clang-format on

		REQUIRE(!RestartOfAnAssociationWithNewAddressesErrorCause::Parse(buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Code:11 (RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES), Length: 7
			0x00, 0x0B, 0x00, 0x07,
			//  NewAddressTlvs: 0x123456 (missing padding byte)
			0x12, 0x34, 0x56,
		};
		// clang-format on

		REQUIRE(!RestartOfAnAssociationWithNewAddressesErrorCause::Parse(buffer2, sizeof(buffer2)));
	}

	SECTION("RestartOfAnAssociationWithNewAddressesErrorCause::Factory() succeeds")
	{
		auto* errorCause = RestartOfAnAssociationWithNewAddressesErrorCause::Factory(
		  FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasNewAddressTlvs() == false);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetNewAddressTlvs(DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasNewAddressTlvs() == true);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 3000);

		errorCause->SetNewAddressTlvs(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasNewAddressTlvs() == false);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetNewAddressTlvs(DataBuffer, 6);

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ errorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasNewAddressTlvs() == true);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 6);
		REQUIRE(errorCause->GetNewAddressTlvs()[0] == 0x00);
		REQUIRE(errorCause->GetNewAddressTlvs()[1] == 0x01);
		REQUIRE(errorCause->GetNewAddressTlvs()[2] == 0x02);
		REQUIRE(errorCause->GetNewAddressTlvs()[3] == 0x03);
		REQUIRE(errorCause->GetNewAddressTlvs()[4] == 0x04);
		REQUIRE(errorCause->GetNewAddressTlvs()[5] == 0x05);
		// These should be padding.
		REQUIRE(errorCause->GetNewAddressTlvs()[6] == 0x00);
		REQUIRE(errorCause->GetNewAddressTlvs()[7] == 0x00);

		/* Parse itself and compare. */

		auto* parsedErrorCause = RestartOfAnAssociationWithNewAddressesErrorCause::Parse(
		  errorCause->GetBuffer(), errorCause->GetLength());

		delete errorCause;

		CHECK_ERROR_CAUSE(
		  /*errorCause*/ parsedErrorCause,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*causeCode*/ ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
		  /*unknownCode*/ false);

		REQUIRE(parsedErrorCause->HasNewAddressTlvs() == true);
		REQUIRE(parsedErrorCause->GetNewAddressTlvsLength() == 6);
		REQUIRE(parsedErrorCause->GetNewAddressTlvs()[0] == 0x00);
		REQUIRE(parsedErrorCause->GetNewAddressTlvs()[1] == 0x01);
		REQUIRE(parsedErrorCause->GetNewAddressTlvs()[2] == 0x02);
		REQUIRE(parsedErrorCause->GetNewAddressTlvs()[3] == 0x03);
		REQUIRE(parsedErrorCause->GetNewAddressTlvs()[4] == 0x04);
		REQUIRE(parsedErrorCause->GetNewAddressTlvs()[5] == 0x05);
		// These should be padding.
		REQUIRE(parsedErrorCause->GetNewAddressTlvs()[6] == 0x00);
		REQUIRE(parsedErrorCause->GetNewAddressTlvs()[7] == 0x00);

		delete parsedErrorCause;
	}
}
