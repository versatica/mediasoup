#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include "RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("IPv4 Adress Parameter (5)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("IPv4AddressParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 8
			0x00, 0x05, 0x00, 0x08,
			// IPv4 Address: "1.2.3.4"
			0x01, 0x02, 0x03, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = RTC::SCTP::IPv4AddressParameter::Parse(buffer, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x01);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x02);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x03);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x04);

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x01);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x02);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x03);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x04);

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
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->GetIPv4Address()[0] == 0x01);
		REQUIRE(clonedParameter->GetIPv4Address()[1] == 0x02);
		REQUIRE(clonedParameter->GetIPv4Address()[2] == 0x03);
		REQUIRE(clonedParameter->GetIPv4Address()[3] == 0x04);

		delete clonedParameter;
	}

	SECTION("IPv4AddressParameter::Parse() fails")
	{
		// Wrong type.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 8
			0x00, 0x06, 0x00, 0x08,
			// IPv4 Address: 0xAABBCCDD
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IPv4AddressParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 7
			0x00, 0x05, 0x00, 0x07,
			// IPv4 Address: 0xAABBCC
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IPv4AddressParameter::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 9
			0x00, 0x05, 0x00, 0x09,
			// IPv4 Address: 0xAABBCCDD
			0xAA, 0xBB, 0xCC, 0xDD,
			0xEE
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IPv4AddressParameter::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 8
			0x00, 0x05, 0x00, 0x08,
			// IPv4 Address (wrong length)
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::IPv4AddressParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("IPv4AddressParameter::Factory() succeeds")
	{
		auto* parameter = RTC::SCTP::IPv4AddressParameter::Factory(
		  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x00);

		/* Modify it. */

		// 11.22.33.44 IPv4 in network order.
		uint8_t ipBuffer[] = { 0x0B, 0x16, 0x21, 0x2C };

		parameter->SetIPv4Address(ipBuffer);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x0B);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x16);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x21);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x2C);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  RTC::SCTP::IPv4AddressParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetIPv4Address()[0] == 0x0B);
		REQUIRE(parsedParameter->GetIPv4Address()[1] == 0x16);
		REQUIRE(parsedParameter->GetIPv4Address()[2] == 0x21);
		REQUIRE(parsedParameter->GetIPv4Address()[3] == 0x2C);

		delete parsedParameter;
	}
}
