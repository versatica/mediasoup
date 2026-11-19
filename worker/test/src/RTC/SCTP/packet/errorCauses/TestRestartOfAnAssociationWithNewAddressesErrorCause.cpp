#include "common.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/RestartOfAnAssociationWithNewAddressesErrorCause.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Restart of an Association with New Addresses Error Cause (11)", "[serializable][sctp][errorcause]")
{
	sctpCommon::ResetBuffers();

	SECTION("RestartOfAnAssociationWithNewAddressesErrorCause::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Code:11 (RESTART-OF-AN-ASSOCIATION-WITH-NEW-ADDRESSES), Length: 11
			0x00, 0x0B, 0x00, 0x0B,
			// New Address TLVs: 0x1234567890AB
			0x12, 0x34, 0x56, 0x78,
			// 1 byte of padding.
			0x90, 0xAB, 0xCD, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause> errorCause{
			RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause::Parse(buffer, sizeof(buffer))
		};

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
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

		/* Serialize it. */

		errorCause->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
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

		errorCause.reset(errorCause->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
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
	}

	SECTION("RestartOfAnAssociationWithNewAddressesErrorCause::Parse() fails")
	{
		// Wrong code.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Code:999 (UNKNOWN), Length: 8
			0x03, 0xE7, 0x00, 0x08,
			//  NewAddressTlvs: 0x12345678
			0x12, 0x34, 0x56, 0x78
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause::Parse(
		  buffer1, sizeof(buffer1)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Code:11 (RESTART-OF-AN-ASSOCIATION-WITH-NEW-ADDRESSES), Length: 7
			0x00, 0x0B, 0x00, 0x07,
			//  NewAddressTlvs: 0x123456 (missing padding byte)
			0x12, 0x34, 0x56
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause::Parse(
		  buffer2, sizeof(buffer2)));
	}

	SECTION("RestartOfAnAssociationWithNewAddressesErrorCause::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause> errorCause{
			RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
		  /*unknownCode*/ false);

		REQUIRE(errorCause->HasNewAddressTlvs() == false);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		errorCause->SetNewAddressTlvs(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(errorCause->GetLength() == 3004);
		REQUIRE(errorCause->HasNewAddressTlvs() == true);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 3000);

		errorCause->SetNewAddressTlvs(nullptr, 0);

		REQUIRE(errorCause->GetLength() == 4);
		REQUIRE(errorCause->HasNewAddressTlvs() == false);
		REQUIRE(errorCause->GetNewAddressTlvsLength() == 0);

		// 6 bytes + 2 bytes of padding.
		errorCause->SetNewAddressTlvs(sctpCommon::DataBuffer, 6);

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
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

		errorCause.reset(
		  RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause::Parse(
		    errorCause->GetBuffer(), errorCause->GetLength()));

		CHECK_SCTP_ERROR_CAUSE(
		  /*errorCause*/ errorCause.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*causeCode*/ RTC::SCTP::ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES,
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
	}
}
