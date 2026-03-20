#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/StateCookieParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("State Cookie Parameter (7)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("StateCookieParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:7 (STATE_COOKIE), Length: 7
			0x00, 0x07, 0x00, 0x07,
			// Cookie: 0xDDCCEE, 1 byte of padding
			0xDD, 0xCC, 0xEE, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = RTC::SCTP::StateCookieParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasCookie() == true);
		REQUIRE(parameter->GetCookieLength() == 3);
		REQUIRE(parameter->GetCookie()[0] == 0xDD);
		REQUIRE(parameter->GetCookie()[1] == 0xCC);
		REQUIRE(parameter->GetCookie()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetCookie()[3] == 0x00);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasCookie() == true);
		REQUIRE(parameter->GetCookieLength() == 3);
		REQUIRE(parameter->GetCookie()[0] == 0xDD);
		REQUIRE(parameter->GetCookie()[1] == 0xCC);
		REQUIRE(parameter->GetCookie()[2] == 0xEE);
		// This should be padding.
		REQUIRE(parameter->GetCookie()[3] == 0x00);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->HasCookie() == true);
		REQUIRE(clonedParameter->GetCookieLength() == 3);
		REQUIRE(clonedParameter->GetCookie()[0] == 0xDD);
		REQUIRE(clonedParameter->GetCookie()[1] == 0xCC);
		REQUIRE(clonedParameter->GetCookie()[2] == 0xEE);
		// This should be padding.
		REQUIRE(clonedParameter->GetCookie()[3] == 0x00);

		delete clonedParameter;
	}

	SECTION("StateCookieParameter::Factory() succeeds (1)")
	{
		auto* parameter = RTC::SCTP::StateCookieParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		/* Modify it. */

		// Verify that replacing the cookie works.
		parameter->SetCookie(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(parameter->GetLength() == 3004);
		REQUIRE(parameter->HasCookie() == true);
		REQUIRE(parameter->GetCookieLength() == 3000);

		parameter->SetCookie(nullptr, 0);

		REQUIRE(parameter->GetLength() == 4);
		REQUIRE(parameter->HasCookie() == false);
		REQUIRE(parameter->GetCookieLength() == 0);

		// 1 bytes + 3 bytes of padding. Note that first (and unique byte) is
		// sctpCommon::DataBuffer + 1 which is initialized to 0x0A.
		parameter->SetCookie(sctpCommon::DataBuffer + 10, 1);

		REQUIRE(parameter->HasCookie() == true);
		REQUIRE(parameter->GetCookieLength() == 1);
		REQUIRE(parameter->GetCookie()[0] == 0x0A);
		// These should be padding.
		REQUIRE(parameter->GetCookie()[1] == 0x00);
		REQUIRE(parameter->GetCookie()[2] == 0x00);
		REQUIRE(parameter->GetCookie()[3] == 0x00);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  RTC::SCTP::StateCookieParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->HasCookie() == true);
		REQUIRE(parsedParameter->GetCookieLength() == 1);
		REQUIRE(parsedParameter->GetCookie()[0] == 0x0A);
		// These should be padding.
		REQUIRE(parsedParameter->GetCookie()[1] == 0x00);
		REQUIRE(parsedParameter->GetCookie()[2] == 0x00);
		REQUIRE(parsedParameter->GetCookie()[3] == 0x00);

		delete parsedParameter;
	}

	SECTION("StateCookieParameter::Factory() succeeds (2)")
	{
		auto* parameter = RTC::SCTP::StateCookieParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		/* Modify it. */

		// Create a StateCookie.
		RTC::SCTP::NegotiatedCapabilities negotiatedCapabilities = { .maxOutboundStreams  = 62000,
			                                                           .maxInboundStreams   = 55555,
			                                                           .partialReliability  = true,
			                                                           .messageInterleaving = true,
			                                                           .reConfig            = true,
			                                                           .zeroChecksum        = false };

		// Build the StateCookie in place within the StateCookieParameter.
		parameter->WriteStateCookieInPlace(
		  /*localVerificationTag*/ 6660666,
		  /*remoteVerificationTag*/ 9990999,
		  /*localInitialTsn*/ 1110111,
		  /*remoteInitialTsn*/ 2220222,
		  /*remoteAdvertisedReceiverWindowCredit*/ 999909999,
		  /*tieTag*/ 1111222233334444,
		  negotiatedCapabilities);

		REQUIRE(parameter->HasCookie() == true);
		REQUIRE(parameter->GetCookieLength() == RTC::SCTP::StateCookie::StateCookieLength);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  RTC::SCTP::StateCookieParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 4 + RTC::SCTP::StateCookie::StateCookieLength,
		  /*length*/ 4 + RTC::SCTP::StateCookie::StateCookieLength,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::STATE_COOKIE,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->HasCookie() == true);
		REQUIRE(parsedParameter->GetCookieLength() == RTC::SCTP::StateCookie::StateCookieLength);

		delete parsedParameter;
	}
}
