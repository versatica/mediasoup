#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "test/include/RTC/SCTP/sctpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("Heartbeat Info Parameter (1)", "[serializable][sctp][parameter]")
{
	sctpCommon::ResetBuffers();

	SECTION("HeartbeatInfoParameter::Parse() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
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
			0xAA
		};
		// clang-format on

		std::unique_ptr<RTC::SCTP::HeartbeatInfoParameter> parameter{
			RTC::SCTP::HeartbeatInfoParameter::Parse(buffer, sizeof(buffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

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

		/* Serialize it. */

		parameter->Serialize(sctpCommon::SerializeBuffer, sizeof(sctpCommon::SerializeBuffer));

		std::memset(buffer, 0x00, sizeof(buffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

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

		parameter.reset(parameter->Clone(sctpCommon::CloneBuffer, sizeof(sctpCommon::CloneBuffer)));

		std::memset(sctpCommon::SerializeBuffer, 0x00, sizeof(sctpCommon::SerializeBuffer));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

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
	}

	SECTION("HeartbeatInfoParameter::Parse() fails")
	{
		// Wrong type.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// Type:6 (IPV6-ADDRESS), Length: 8
			0x00, 0x06, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::HeartbeatInfoParameter::Parse(buffer1, sizeof(buffer1)));

		// Wrong length field.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// Type:1 (HEARBEAT_INFO), Length: 3
			0x00, 0x01, 0x00, 0x03,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding
			0x55, 0x66, 0x77, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::HeartbeatInfoParameter::Parse(buffer2, sizeof(buffer2)));

		// Wrong buffer length.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// Type:1 (HEARBEAT_INFO), Length: 11
			0x00, 0x01, 0x00, 0x0B,
			// Heartbeat Information (7 bytes): 0x11223344556677
			0x11, 0x22, 0x33, 0x44,
			// 1 byte of padding (missing)
			0x55, 0x66, 0x77
		};
		// clang-format on

		REQUIRE(!RTC::SCTP::HeartbeatInfoParameter::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("HeartbeatInfoParameter::Factory() succeeds")
	{
		std::unique_ptr<RTC::SCTP::HeartbeatInfoParameter> parameter{
			RTC::SCTP::HeartbeatInfoParameter::Factory(
			  sctpCommon::FactoryBuffer, sizeof(sctpCommon::FactoryBuffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE(parameter->HasInfo() == false);
		REQUIRE(parameter->GetInfoLength() == 0);

		/* Modify it. */

		// Verify that replacing the value works.
		parameter->SetInfo(sctpCommon::DataBuffer + 1000, 3000);

		REQUIRE(parameter->GetLength() == 3004);
		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 3000);

		parameter->SetInfo(nullptr, 0);

		REQUIRE(parameter->GetLength() == 4);
		REQUIRE(parameter->HasInfo() == false);
		REQUIRE(parameter->GetInfoLength() == 0);

		parameter->SetInfo(sctpCommon::DataBuffer, 2);

		REQUIRE(parameter->GetLength() == 8);
		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 2);

		parameter->SetInfo(sctpCommon::DataBuffer + 2000, 2000);

		REQUIRE(parameter->GetLength() == 2004);
		REQUIRE(parameter->HasInfo() == true);
		REQUIRE(parameter->GetInfoLength() == 2000);

		// Info length is 5 so 3 bytes of padding will be added.
		parameter->SetInfo(sctpCommon::DataBuffer, 5);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

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

		parameter.reset(
		  RTC::SCTP::HeartbeatInfoParameter::Parse(parameter->GetBuffer(), parameter->GetLength()));

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::FactoryBuffer,
		  /*bufferLength*/ 12,
		  /*length*/ 12,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

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
	}

	SECTION("HeartbeatInfoParameter::SetInfo() throws if infoLength is too big")
	{
		const std::unique_ptr<RTC::SCTP::HeartbeatInfoParameter> parameter{
			RTC::SCTP::HeartbeatInfoParameter::Factory(
			  sctpCommon::ThrowBuffer, sizeof(sctpCommon::ThrowBuffer))
		};

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::ThrowBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::ThrowBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);

		REQUIRE_THROWS_AS(parameter->SetInfo(sctpCommon::ThrowBuffer, 65535), MediaSoupError);

		CHECK_SCTP_PARAMETER(
		  /*parameter*/ parameter.get(),
		  /*buffer*/ sctpCommon::ThrowBuffer,
		  /*bufferLength*/ sizeof(sctpCommon::ThrowBuffer),
		  /*length*/ 4,
		  /*parameterType*/ RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO,
		  /*unknownType*/ false,
		  /*actionForUnknownParameterType*/ RTC::SCTP::Parameter::ActionForUnknownParameterType::STOP);
	}
}
