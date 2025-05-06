#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("IPv4 Adress Parameter (5)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("IPv4AddressParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
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

		auto* parameter = IPv4AddressParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x01);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x02);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x03);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x04);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetIPv4Address(ThrowBuffer), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x01);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x02);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x03);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x04);

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

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
		uint8_t buffer1[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 8
			0x00, 0x06, 0x00, 0x08,
			// IPv4 Address: 0xAABBCCDD
			0xAA, 0xBB, 0xCC, 0xDD
		};
		// clang-format on

		REQUIRE(!IPv4AddressParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 7
			0x00, 0x05, 0x00, 0x07,
			// IPv4 Address: 0xAABBCC
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		REQUIRE(!IPv4AddressParameter::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer3[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 9
			0x00, 0x05, 0x00, 0x09,
			// IPv4 Address: 0xAABBCCDD
			0xAA, 0xBB, 0xCC, 0xDD,
			0xEE
		};
		// clang-format on

		REQUIRE(!IPv4AddressParameter::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 8
			0x00, 0x05, 0x00, 0x08,
			// IPv4 Address (wrong length)
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		REQUIRE(!IPv4AddressParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("IPv4AddressParameter::Factory() succeeds")
	{
		auto* parameter = IPv4AddressParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x00);

		/* Modify it. */

		// 11.22.33.44 IPv4 in network order.
		uint8_t ipBuffer[] = { 0x0B, 0x16, 0x21, 0x2C };

		parameter->SetIPv4Address(ipBuffer);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x0B);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x16);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x21);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x2C);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  IPv4AddressParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 8,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetIPv4Address()[0] == 0x0B);
		REQUIRE(parsedParameter->GetIPv4Address()[1] == 0x16);
		REQUIRE(parsedParameter->GetIPv4Address()[2] == 0x21);
		REQUIRE(parsedParameter->GetIPv4Address()[3] == 0x2C);

		delete parsedParameter;
	}
}
