#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/common.hpp" // in worker/test/include/
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

using namespace RTC::SCTP;

SCENARIO("Heartbeat Info Parameter (1)", "[sctp][serializable]")
{
	resetBuffers();

	SECTION("HeartbeatInfoParameter::Parse() succeeds")
	{
		// clang-format off
		uint8_t buffer[] =
		{
			// Type:1 (HEARBEAT_INFO), Length: 11
			0x00, 0x01, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
			// Extra bytes that should be ignored
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA, 0xBB, 0xCC, 0xDD,
			0xAA,
		};
		// clang-format on

		auto* parameter = HeartbeatInfoParameter::Parse(buffer, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 7);
		REQUIRE(parameter->GetInfo()[0] == 0x11);
		REQUIRE(parameter->GetInfo()[1] == 0x22);
		REQUIRE(parameter->GetInfo()[2] == 0x33);
		REQUIRE(parameter->GetInfo()[3] == 0x44);
		REQUIRE(parameter->GetInfo()[4] == 0x55);
		REQUIRE(parameter->GetInfo()[5] == 0x66);
		REQUIRE(parameter->GetInfo()[6] == 0x77);
		// This should be padding.
		REQUIRE(parameter->GetInfo()[7] == 0x00);

		/* Should throw if modifications are attempted when it's frozen. */

		REQUIRE_THROWS_AS(parameter->SetInfo(DataBuffer, 5), MediaSoupError);

		/* Serialize it. */

		parameter->Serialize(SerializeBuffer, sizeof(SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ SerializeBuffer,
		  /*bufferLength*/ sizeof(SerializeBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 7);
		REQUIRE(parameter->GetInfo()[0] == 0x11);
		REQUIRE(parameter->GetInfo()[1] == 0x22);
		REQUIRE(parameter->GetInfo()[2] == 0x33);
		REQUIRE(parameter->GetInfo()[3] == 0x44);
		REQUIRE(parameter->GetInfo()[4] == 0x55);
		REQUIRE(parameter->GetInfo()[5] == 0x66);
		REQUIRE(parameter->GetInfo()[6] == 0x77);
		// This should be padding.
		REQUIRE(parameter->GetInfo()[7] == 0x00);

		/* Clone it. */

		auto* clonedParameter = parameter->Clone(CloneBuffer, sizeof(CloneBuffer));

		std::memset(SerializeBuffer, 0x00, sizeof(SerializeBuffer));

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ clonedParameter,
		  /*buffer*/ CloneBuffer,
		  /*bufferLength*/ sizeof(CloneBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(clonedParameter->HasInfo() == true);
		REQUIRE(clonedParameter->GetInfoLength() == 7);
		REQUIRE(clonedParameter->GetInfo()[0] == 0x11);
		REQUIRE(clonedParameter->GetInfo()[1] == 0x22);
		REQUIRE(clonedParameter->GetInfo()[2] == 0x33);
		REQUIRE(clonedParameter->GetInfo()[3] == 0x44);
		REQUIRE(clonedParameter->GetInfo()[4] == 0x55);
		REQUIRE(clonedParameter->GetInfo()[5] == 0x66);
		REQUIRE(clonedParameter->GetInfo()[6] == 0x77);
		// This should be padding.
		REQUIRE(clonedParameter->GetInfo()[7] == 0x00);

		delete clonedParameter;
	}

	SECTION("HeartbeatInfoParameter::Parse() fails")
	{
		// Wrong type.
		// clang-format off
		uint8_t buffer1[] =
		{
			// Type:6 (IPV6_ADDRESS), Length: 8
			0x00, 0x06, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
		};
		// clang-format on

		REQUIRE(!HeartbeatInfoParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong Length field.
		// clang-format off
		uint8_t buffer2[] =
		{
			// Type:1 (HEARBEAT_INFO), Length: 3
			0x00, 0x01, 0x00, 0x03,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00,
		};
		// clang-format on

		REQUIRE(!HeartbeatInfoParameter::Parse(buffer2, sizeof(buffer2)));

		// Wrong buffer length.
		// clang-format off
		uint8_t buffer4[] =
		{
			// Type:1 (HEARBEAT_INFO), Length: 11
			0x00, 0x01, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding (missing)
			0x55, 0x66, 0x77
		};
		// clang-format on

		REQUIRE(!HeartbeatInfoParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("HeartbeatInfoParameter::Factory() succeeds")
	{
		auto* parameter = HeartbeatInfoParameter::Factory(FactoryBuffer, sizeof(FactoryBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasInfo() == false);
		REQUIRE(parameter->GetInfoLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		parameter->SetInfo(DataBuffer + 1000, 3000);

		REQUIRE(parameter->GetLength() == 3004);
		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 3000);

		parameter->SetInfo(nullptr, 0);

		REQUIRE(parameter->GetLength() == 4);
		REQUIRE(parameter->HasInfo() == false);
		REQUIRE(parameter->GetInfoLength() == 0);

		parameter->SetInfo(DataBuffer, 2);

		REQUIRE(parameter->GetLength() == 8);
		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 2);

		parameter->SetInfo(DataBuffer + 2000, 2000);

		REQUIRE(parameter->GetLength() == 2004);
		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 2000);

		// Info length is 5 so 3 bytes of padding will be added.
		parameter->SetInfo(DataBuffer, 5);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ sizeof(FactoryBuffer),
		  /*length*/ 12,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 5);
		REQUIRE(parameter->GetInfo()[0] == 0x00);
		REQUIRE(parameter->GetInfo()[1] == 0x01);
		REQUIRE(parameter->GetInfo()[2] == 0x02);
		REQUIRE(parameter->GetInfo()[3] == 0x03);
		REQUIRE(parameter->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(parameter->GetInfo()[5] == 0x00);
		REQUIRE(parameter->GetInfo()[6] == 0x00);
		REQUIRE(parameter->GetInfo()[7] == 0x00);

		/* Parse itself and compare. */

		auto* parsedParameter =
		  HeartbeatInfoParameter::Parse(parameter->GetBuffer(), parameter->GetLength());

		delete parameter;

		CHECK_PARAMETER(
		  /*parameter*/ parsedParameter,
		  /*buffer*/ FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*frozen*/ true,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parsedParameter->HasInfo() == true);
		REQUIRE(parsedParameter->GetInfoLength() == 5);
		REQUIRE(parsedParameter->GetInfo()[0] == 0x00);
		REQUIRE(parsedParameter->GetInfo()[1] == 0x01);
		REQUIRE(parsedParameter->GetInfo()[2] == 0x02);
		REQUIRE(parsedParameter->GetInfo()[3] == 0x03);
		REQUIRE(parsedParameter->GetInfo()[4] == 0x04);
		// These should be padding.
		REQUIRE(parsedParameter->GetInfo()[5] == 0x00);
		REQUIRE(parsedParameter->GetInfo()[6] == 0x00);
		REQUIRE(parsedParameter->GetInfo()[7] == 0x00);

		delete parsedParameter;
	}

	SECTION("HeartbeatInfoParameter::SetInfo() throws if infoLength is too big")
	{
		auto* parameter = HeartbeatInfoParameter::Factory(ThrowBuffer, sizeof(ThrowBuffer));

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE_THROWS_AS(parameter->SetInfo(ThrowBuffer, 65535), MediaSoupError);

		CHECK_PARAMETER(
		  /*parameter*/ parameter,
		  /*buffer*/ ThrowBuffer,
		  /*bufferLength*/ sizeof(ThrowBuffer),
		  /*length*/ 4,
		  /*frozen*/ false,
		  /*parameterType*/ Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ Parameter::ActionForUnknownParameterType::STOP);

		delete parameter;
	}
}
