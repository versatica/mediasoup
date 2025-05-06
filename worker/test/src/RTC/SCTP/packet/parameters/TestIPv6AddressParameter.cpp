#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv6AddressParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("IPv6 Adress Parameter (6)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("IPv6AddressParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
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

		auto* parameter = IPv6AddressParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter->GetIPv6Address()[15] == 0x34);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetIPv6Address(ThrowBuffer), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter->GetIPv6Address()[15] == 0x34);

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

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
		uint8_t buffer1[] =
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

		REQUIRE(!IPv6AddressParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
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

		REQUIRE(!IPv6AddressParameter::Parse(buffer2, sizeof(buffer2)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer3[] =
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

		REQUIRE(!IPv6AddressParameter::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
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

		REQUIRE(!IPv6AddressParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("IPv6AddressParameter::Factory() succeeds")
	{
		auto* parameter = IPv6AddressParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

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

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x23);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x45);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x04);
		REQUIRE(parameter->GetIPv6Address()[3] == 0x25);
		REQUIRE(parameter->GetIPv6Address()[15] == 0xB5);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  IPv6AddressParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 20,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->GetIPv6Address()[0] == 0x23);
		REQUIRE(parsedParameter->GetIPv6Address()[1] == 0x45);
		REQUIRE(parsedParameter->GetIPv6Address()[2] == 0x04);
		REQUIRE(parsedParameter->GetIPv6Address()[3] == 0x25);
		REQUIRE(parsedParameter->GetIPv6Address()[15] == 0xB5);

		delete parsedParameter;
	}
}
