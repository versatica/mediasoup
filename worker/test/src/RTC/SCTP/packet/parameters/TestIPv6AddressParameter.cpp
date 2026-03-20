#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv6AddressParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("IPv6 Adress Parameter (6)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("IPv6AddressParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 20
			0x00, 0x06, 0x00, 0x14,
			// IPv6 Address: "2001:0db8:85a3:0000:0000:8a2e:0370:7334"
			0x20, 0x01, 0x0D, 0xB8,
			0x85, 0xA3, 0x00, 0x00,
			0x00, 0x00, 0x8A, 0x2E,
			0x03, 0x70, 0x73, 0x34,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = RTC::SCTP::IPv6AddressParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter->GetIPv6Address()[15] == 0x34);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter->GetIPv6Address()[15] == 0x34);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetIPv6Address()[0] == 0x20);
		REQUIRE(clonedParameter->GetIPv6Address()[1] == 0x01);
		REQUIRE(clonedParameter->GetIPv6Address()[2] == 0x0D);
		REQUIRE(clonedParameter->GetIPv6Address()[3] == 0xB8);
		REQUIRE(clonedParameter->GetIPv6Address()[15] == 0x34);

		delete clonedParameter;
	}

	SECTION("IPv6AddressParameter::Parse() fails")
	{
		// Wrong type.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 20
			0x00, 0x05, 0x00, 0x14,
			// IPv6 Address: "2001:0db8:85a3:0000:0000:8a2e:0370:7334"
			0x20, 0x01, 0x0D, 0xB8,
			0x85, 0xA3, 0x00, 0x00,
			0x00, 0x00, 0x8A, 0x2E,
			0x03, 0x70, 0x73, 0x34,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IPv6AddressParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 19
			0x00, 0x06, 0x00, 0x14,
			// IPv6 Address: "2001:0db8:85a3:0000:0000:8a2e:0370:7334"
			0x20, 0x01, 0x0D, 0xB8,
			0x85, 0xA3, 0x00, 0x00,
			0x00, 0x00, 0x8A, 0x2E,
			0x03, 0x70, 0x73,
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IPv6AddressParameter::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 21
			0x00, 0x06, 0x00, 0x15,
			// IPv6 Address: "2001:0db8:85a3:0000:0000:8a2e:0370:7334"
			0x20, 0x01, 0x0D, 0xB8,
			0x85, 0xA3, 0x00, 0x00,
			0x00, 0x00, 0x8A, 0x2E,
			0x03, 0x70, 0x73, 0x34,
			0x00
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IPv6AddressParameter::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 20
			0x00, 0x06, 0x00, 0x14,
			// IPv6 Address (wrong length)
			0x20, 0x01, 0x0D, 0xB8,
			0x85, 0xA3, 0x00, 0x00,
			0x00, 0x00, 0x8A, 0x2E,
			0x03, 0x70, 0x73
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IPv6AddressParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("IPv6AddressParameter::Factory() succeeds")
	{
		auto* parameter = RTC::SCTP::IPv6AddressParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x00);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x00);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x00);
		REQUIRE(parameter->GetIPv6Address()[3] == 0x00);
		REQUIRE(parameter->GetIPv6Address()[15] == 0x00);

		/* Modify it. */

		// 2345:0425:2CA1:0000:0000:0567:5673:23b5 IPv6 in network order.
		uint8_t ipBuffer[] = { 0x23, 0x45, 0x04, 0x25, 0x2C, 0xA1, 0x00, 0x00,
			                     0x00, 0x00, 0x05, 0x67, 0x56, 0x73, 0x23, 0xB5 };

		parameter->SetIPv6Address(ipBuffer);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x23);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x45);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x04);
		REQUIRE(parameter->GetIPv6Address()[3] == 0x25);
		REQUIRE(parameter->GetIPv6Address()[15] == 0xB5);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  RTC::SCTP::IPv6AddressParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetIPv6Address()[0] == 0x23);
		REQUIRE(parsedParameter->GetIPv6Address()[1] == 0x45);
		REQUIRE(parsedParameter->GetIPv6Address()[2] == 0x04);
		REQUIRE(parsedParameter->GetIPv6Address()[3] == 0x25);
		REQUIRE(parsedParameter->GetIPv6Address()[15] == 0xB5);

		delete parsedParameter;
	}
}
