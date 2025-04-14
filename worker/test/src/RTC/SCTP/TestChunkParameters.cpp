#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "helpers.hpp"
#include "RTC/SCTP/ChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/HeartbeatInfoChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/IPv4AddressChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/IPv6AddressChunkParameter.hpp"
#include "RTC/SCTP/chunkParameters/UnknownChunkParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

thread_local static uint8_t ChunkParameterFactoryBuffer[66661];
thread_local static uint8_t ChunkParameterSerializeBuffer[66662];
thread_local static uint8_t ChunkParameterCloneBuffer[66663];
thread_local static uint8_t ChunkParameterCustomDataBuffer[66664];
thread_local static uint8_t ThrowBuffer[66665];

static void resetBuffers();

static void checkChunkParameter(
  const ChunkParameter* parameter,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  ChunkParameter::ChunkParameterType parameterType,
  bool unknownType,
  ChunkParameter::ActionForUnknownChunkParameterType actionForUnknownParameterType,
  uint16_t valueLength);

SCENARIO("HeartbeatInfo Chunk Parameter (1)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("HeartbeatInfoChunkParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:1 (HEARBEAT_INFO), Length: 11
			0x00, 0x01, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677, 1 byte of padding
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = HeartbeatInfoChunkParameter::Parse(buffer, sizeof(buffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ 15,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter->GetInfo()[0] == 0x11);
		REQUIRE(parameter->GetInfo()[1] == 0x22);
		REQUIRE(parameter->GetInfo()[2] == 0x33);
		REQUIRE(parameter->GetInfo()[3] == 0x44);
		REQUIRE(parameter->GetInfo()[4] == 0x55);
		REQUIRE(parameter->GetInfo()[5] == 0x66);
		REQUIRE(parameter->GetInfo()[6] == 0x77);

		/* Serialize it. */

		parameter->Serialize(ChunkParameterSerializeBuffer, sizeof(ChunkParameterSerializeBuffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterSerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(parameter->GetInfo()[0] == 0x11);
		REQUIRE(parameter->GetInfo()[1] == 0x22);
		REQUIRE(parameter->GetInfo()[2] == 0x33);
		REQUIRE(parameter->GetInfo()[3] == 0x44);
		REQUIRE(parameter->GetInfo()[4] == 0x55);
		REQUIRE(parameter->GetInfo()[5] == 0x66);
		REQUIRE(parameter->GetInfo()[6] == 0x77);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(ChunkParameterCloneBuffer, sizeof(ChunkParameterCloneBuffer));

		delete parameter;

		checkChunkParameter(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ ChunkParameterCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterCloneBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 7);

		REQUIRE(clonedParameter->GetInfo()[0] == 0x11);
		REQUIRE(clonedParameter->GetInfo()[1] == 0x22);
		REQUIRE(clonedParameter->GetInfo()[2] == 0x33);
		REQUIRE(clonedParameter->GetInfo()[3] == 0x44);
		REQUIRE(clonedParameter->GetInfo()[4] == 0x55);
		REQUIRE(clonedParameter->GetInfo()[5] == 0x66);
		REQUIRE(clonedParameter->GetInfo()[6] == 0x77);

		delete clonedParameter;
	}

	SECTION("HeartbeatInfoChunkParameter::Parse() fails")
	{
		// Wrong type.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 8
			0x00, 0x06, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677, 1 byte of padding
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
		};
		// clang-format on

		REQUIRE(!HeartbeatInfoChunkParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Type:1 (HEARBEAT_INFO), Length: 3
			0x00, 0x01, 0x00, 0x03,
			// Heartbeat Information (7 bytes): 0x11223344556677, 1 byte of padding
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
		};
		// clang-format on

		REQUIRE(!HeartbeatInfoChunkParameter::Parse(buffer2, sizeof(buffer2)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Type:1 (HEARBEAT_INFO), Length: 11
			0x00, 0x01, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677, 1 byte of padding
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77
		};
		// clang-format on

		REQUIRE(!HeartbeatInfoChunkParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("HeartbeatInfoChunkParameter::Factory() succeeds")
	{
		auto* parameter = HeartbeatInfoChunkParameter::Factory(
		  ChunkParameterFactoryBuffer, sizeof(ChunkParameterFactoryBuffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterFactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 0);

		/* Modify it. */

		// Info length is 5 so 3 bytes of padding will be added.
		parameter->SetInfo(ChunkParameterCustomDataBuffer, 5);

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterFactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 5);

		REQUIRE(parameter->GetInfo()[0] == 0x00);
		REQUIRE(parameter->GetInfo()[1] == 0x01);
		REQUIRE(parameter->GetInfo()[2] == 0x02);
		REQUIRE(parameter->GetInfo()[3] == 0x03);
		REQUIRE(parameter->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(parameter->GetInfo()[5] == 0x00);
		REQUIRE(parameter->GetInfo()[6] == 0x00);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  HeartbeatInfoChunkParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		checkChunkParameter(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ parameter->GetLength(),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 5);

		REQUIRE(parsedParameter->GetInfo()[0] == 0x00);
		REQUIRE(parsedParameter->GetInfo()[1] == 0x01);
		REQUIRE(parsedParameter->GetInfo()[2] == 0x02);
		REQUIRE(parsedParameter->GetInfo()[3] == 0x03);
		REQUIRE(parsedParameter->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(parsedParameter->GetInfo()[5] == 0x00);
		REQUIRE(parsedParameter->GetInfo()[6] == 0x00);

		delete parameter;
		delete parsedParameter;
	}

	SECTION("HeartbeatInfoChunkParameter::SetInfo() throws if infoLength is too big")
	{
		auto* parameter = HeartbeatInfoChunkParameter::Factory(ThrowBuffer, sizeof(ThrowBuffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 0);

		REQUIRE_THROWS_AS(parameter->SetInfo(ThrowBuffer, 65535), MediaSoupError);

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 0);

		delete parameter;
	}
}

SCENARIO("IPv4 Adress Chunk Parameter (5)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("IPv4AddressChunkParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 8
			0x00, 0x05, 0x00, 0x08,
			// IPv4 Address: "1.2.3.4"
			0x01, 0x02, 0x03, 0x04,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		auto* parameter = IPv4AddressChunkParameter::Parse(buffer, sizeof(buffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ 11,
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x01);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x02);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x03);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x04);

		/* Serialize it. */

		parameter->Serialize(ChunkParameterSerializeBuffer, sizeof(ChunkParameterSerializeBuffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterSerializeBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x01);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x02);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x03);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x04);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(ChunkParameterCloneBuffer, sizeof(ChunkParameterCloneBuffer));

		delete parameter;

		checkChunkParameter(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ ChunkParameterCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterCloneBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(clonedParameter->GetIPv4Address()[0] == 0x01);
		REQUIRE(clonedParameter->GetIPv4Address()[1] == 0x02);
		REQUIRE(clonedParameter->GetIPv4Address()[2] == 0x03);
		REQUIRE(clonedParameter->GetIPv4Address()[3] == 0x04);

		delete clonedParameter;
	}

	SECTION("IPv4AddressChunkParameter::Parse() fails")
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

		REQUIRE(!IPv4AddressChunkParameter::Parse(buffer1, sizeof(buffer1)));

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

		REQUIRE(!IPv4AddressChunkParameter::Parse(buffer2, sizeof(buffer2)));

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

		REQUIRE(!IPv4AddressChunkParameter::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Type:5 (IPV4_ADDRESS), Length: 8
			0x00, 0x05, 0x00, 0x08,
			// IPv4 Address: 0xAABBCC
			0xAA, 0xBB, 0xCC
		};
		// clang-format on

		REQUIRE(!IPv4AddressChunkParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("IPv4AddressChunkParameter::Factory() succeeds")
	{
		auto* parameter = IPv4AddressChunkParameter::Factory(
		  ChunkParameterFactoryBuffer, sizeof(ChunkParameterFactoryBuffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterFactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x00);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x00);

		/* Modify it. */

		// 11.22.33.44 IPv4 in network order.
		uint8_t ipBuffer[] = { 0x0B, 0x16, 0x21, 0x2C };

		parameter->SetIPv4Address(ipBuffer);

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterFactoryBuffer),
		  /*length*/ 8,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parameter->GetIPv4Address()[0] == 0x0B);
		REQUIRE(parameter->GetIPv4Address()[1] == 0x16);
		REQUIRE(parameter->GetIPv4Address()[2] == 0x21);
		REQUIRE(parameter->GetIPv4Address()[3] == 0x2C);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  IPv4AddressChunkParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		checkChunkParameter(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ parameter->GetLength(),
		  /*length*/ 8,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 4);

		REQUIRE(parsedParameter->GetIPv4Address()[0] == 0x0B);
		REQUIRE(parsedParameter->GetIPv4Address()[1] == 0x16);
		REQUIRE(parsedParameter->GetIPv4Address()[2] == 0x21);
		REQUIRE(parsedParameter->GetIPv4Address()[3] == 0x2C);

		delete parameter;
		delete parsedParameter;
	}
}

SCENARIO("IPv6 Adress Chunk Parameter (6)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("IPv6AddressChunkParameter::Parse() succeeds")
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

		auto* parameter = IPv6AddressChunkParameter::Parse(buffer, sizeof(buffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ 23,
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter->GetIPv6Address()[15] == 0x34);

		/* Serialize it. */

		parameter->Serialize(ChunkParameterSerializeBuffer, sizeof(ChunkParameterSerializeBuffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterSerializeBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterSerializeBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x20);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x01);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x0D);
		REQUIRE(parameter->GetIPv6Address()[3] == 0xB8);
		REQUIRE(parameter->GetIPv6Address()[15] == 0x34);

		/* Clone it. */

		auto* clonedParameter =
		  parameter->Clone(ChunkParameterCloneBuffer, sizeof(ChunkParameterCloneBuffer));

		delete parameter;

		checkChunkParameter(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ ChunkParameterCloneBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterCloneBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(clonedParameter->GetIPv6Address()[0] == 0x20);
		REQUIRE(clonedParameter->GetIPv6Address()[1] == 0x01);
		REQUIRE(clonedParameter->GetIPv6Address()[2] == 0x0D);
		REQUIRE(clonedParameter->GetIPv6Address()[3] == 0xB8);
		REQUIRE(clonedParameter->GetIPv6Address()[15] == 0x34);

		delete clonedParameter;
	}

	SECTION("IPv6AddressChunkParameter::Parse() fails")
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

		REQUIRE(!IPv6AddressChunkParameter::Parse(buffer1, sizeof(buffer1)));

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

		REQUIRE(!IPv6AddressChunkParameter::Parse(buffer2, sizeof(buffer2)));

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

		REQUIRE(!IPv6AddressChunkParameter::Parse(buffer3, sizeof(buffer3)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 20
			0x00, 0x06, 0x00, 0x14,
			// IPv6 Address: "2001:0db8:85a3:0000:0000:8a2e:0370:7334"
			0x20, 0x01, 0x0D, 0xB8,
			0x85, 0xA3, 0x00, 0x00,
			0x00, 0x00, 0x8A, 0x2E,
			0x03, 0x70, 0x73
		};
		// clang-format on

		REQUIRE(!IPv6AddressChunkParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("IPv6AddressChunkParameter::Factory() succeeds")
	{
		auto* parameter = IPv6AddressChunkParameter::Factory(
		  ChunkParameterFactoryBuffer, sizeof(ChunkParameterFactoryBuffer));

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterFactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

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

		checkChunkParameter(
		  /*parameter*/ parameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ sizeof(ChunkParameterFactoryBuffer),
		  /*length*/ 20,
		  /*frozen*/ false,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(parameter->GetIPv6Address()[0] == 0x23);
		REQUIRE(parameter->GetIPv6Address()[1] == 0x45);
		REQUIRE(parameter->GetIPv6Address()[2] == 0x04);
		REQUIRE(parameter->GetIPv6Address()[3] == 0x25);
		REQUIRE(parameter->GetIPv6Address()[15] == 0xB5);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  IPv6AddressChunkParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		checkChunkParameter(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ ChunkParameterFactoryBuffer,
		  /*bufferLength*/ parameter->GetLength(),
		  /*length*/ 20,
		  /*frozen*/ true,
		  /*parameterType*/ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ ChunkParameter::ActionForUnknownChunkParameterType::STOP,
		  /*valueLength*/ 16);

		REQUIRE(parsedParameter->GetIPv6Address()[0] == 0x23);
		REQUIRE(parsedParameter->GetIPv6Address()[1] == 0x45);
		REQUIRE(parsedParameter->GetIPv6Address()[2] == 0x04);
		REQUIRE(parsedParameter->GetIPv6Address()[3] == 0x25);
		REQUIRE(parsedParameter->GetIPv6Address()[15] == 0xB5);

		delete parameter;
		delete parsedParameter;
	}
}

// TODO:
// SCENARIO("Unknown Chunk Parameter", "[sctp][serializable]")

void resetBuffers()
{
	std::memset(ChunkParameterFactoryBuffer, 0xAA, sizeof(ChunkParameterFactoryBuffer));
	std::memset(ChunkParameterSerializeBuffer, 0xBB, sizeof(ChunkParameterSerializeBuffer));
	std::memset(ChunkParameterCloneBuffer, 0xCC, sizeof(ChunkParameterCloneBuffer));
	std::memset(ChunkParameterCustomDataBuffer, 0xDD, sizeof(ChunkParameterCustomDataBuffer));
	std::memset(ThrowBuffer, 0xEE, sizeof(ThrowBuffer));

	ChunkParameterCustomDataBuffer[0] = 0x00;
	ChunkParameterCustomDataBuffer[1] = 0x01;
	ChunkParameterCustomDataBuffer[2] = 0x02;
	ChunkParameterCustomDataBuffer[3] = 0x03;
	ChunkParameterCustomDataBuffer[4] = 0x04;
	ChunkParameterCustomDataBuffer[5] = 0x05;
	ChunkParameterCustomDataBuffer[6] = 0x06;
	ChunkParameterCustomDataBuffer[7] = 0x07;
}

static void checkChunkParameter(
  const ChunkParameter* parameter,
  const uint8_t* buffer,
  size_t bufferLength,
  size_t length,
  bool frozen,
  ChunkParameter::ChunkParameterType parameterType,
  bool unknownType,
  ChunkParameter::ActionForUnknownChunkParameterType actionForUnknownParameterType,
  uint16_t valueLength)
{
	REQUIRE(parameter);
	REQUIRE(parameter->GetBuffer() == buffer);
	REQUIRE(parameter->GetBufferLength() == bufferLength);
	REQUIRE(parameter->GetLength() == length);
	REQUIRE(parameter->IsFrozen() == frozen);
	REQUIRE(parameter->GetType() == parameterType);
	REQUIRE(parameter->HasUnknownType() == unknownType);
	REQUIRE(parameter->GetActionForUnknownChunkParameterType() == actionForUnknownParameterType);
	REQUIRE(parameter->HasValue() == valueLength > 0);
	REQUIRE(parameter->GetValueLength() == valueLength);
	REQUIRE(
	  helpers::areBuffersEqual(parameter->GetBuffer(), parameter->GetLength(), buffer, length) == true);

	// Also assert that Serialize() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, length - 1), MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, std::min<size_t>(3, length - 1)),
	  MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, std::min<size_t>(7, length - 1)),
	  MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, std::min<size_t>(11, length - 2)),
	  MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, std::min<size_t>(15, length - 2)),
	  MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, std::min<size_t>(19, length - 3)),
	  MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, std::min<size_t>(23, length - 3)),
	  MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, std::min<size_t>(27, length - 4)),
	  MediaSoupError);
	REQUIRE_THROWS_AS(
	  const_cast<ChunkParameter*>(parameter)->Serialize(ThrowBuffer, std::min<size_t>(31, length - 4)),
	  MediaSoupError);

	// Also assert that Clone() throws if a too small buffer is given.
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, length - 1), MediaSoupError);
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, std::min<size_t>(3, length - 1)), MediaSoupError);
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, std::min<size_t>(7, length - 1)), MediaSoupError);
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, std::min<size_t>(11, length - 2)), MediaSoupError);
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, std::min<size_t>(15, length - 2)), MediaSoupError);
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, std::min<size_t>(19, length - 3)), MediaSoupError);
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, std::min<size_t>(23, length - 3)), MediaSoupError);
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, std::min<size_t>(27, length - 3)), MediaSoupError);
	REQUIRE_THROWS_AS(parameter->Clone(ThrowBuffer, std::min<size_t>(31, length - 1)), MediaSoupError);
}
