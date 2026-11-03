#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/NEW_RTCP/packet/ExtendedReport.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/RrtExtendedReport.hpp"
#include "Utils.hpp"
#include "test/include/RTC/NEW_RTCP/rtcpCommon.hpp"
#include "test/include/testHelpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP XR Receiver Reference Time Extended Report (4)", "[serializable][rtcp][extendedreport]")
{
	rtcpCommon::ResetBuffers();

	SECTION("RrtExtendedReport::Parse() succeeds")
	{
		const auto checkExtendedReport = [](const RTC::NEW_RTCP::RrtExtendedReport* extendedReport) -> void
		{
			const Utils::Time::Ntp ntp = extendedReport->GetNtp();

			REQUIRE(ntp.seconds == 0x12345678);
			REQUIRE(ntp.fractions == 0x23456789);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// BT: 4 (RRT), Reserved: 0, Block length: 2
			0x04, 0x00, 0x00, 0x02,
			// NTP timestamp, most significant word: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// NTP timestamp, least significant word: 0x23456789
			0x23, 0x45, 0x67, 0x89
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::RrtExtendedReport> extendedReport{
			RTC::NEW_RTCP::RrtExtendedReport::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::RRT,
		  /*unknownType*/ false);

		checkExtendedReport(extendedReport.get());

		/* Serialize it. */

		extendedReport->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 12,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::RRT,
		  /*unknownType*/ false);

		checkExtendedReport(extendedReport.get());

		/* Clone it. */

		extendedReport.reset(
		  extendedReport->Clone(rtcpCommon::CloneBuffer, sizeof(rtcpCommon::CloneBuffer)));

		std::memset(rtcpCommon::SerializeBuffer, 0xFF, sizeof(rtcpCommon::SerializeBuffer));

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ rtcpCommon::CloneBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::CloneBuffer),
		  /*length*/ 12,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::RRT,
		  /*unknownType*/ false);

		checkExtendedReport(extendedReport.get());
	}

	SECTION("RrtExtendedReport::Parse() fails")
	{
		// Block length field is not 2.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			// BT: 4 (RRT), Reserved: 0, Block length: 3
			0x04, 0x00, 0x00, 0x03,
			0x12, 0x34, 0x56, 0x78,
			0x23, 0x45, 0x67, 0x89,
			0x00, 0x00, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RrtExtendedReport::Parse(buffer1, sizeof(buffer1)));

		// Block length field is not 2.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// BT: 4 (RRT), Reserved: 0, Block length: 1
			0x04, 0x00, 0x00, 0x01,
			0x12, 0x34, 0x56, 0x78
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RrtExtendedReport::Parse(buffer2, sizeof(buffer2)));

		// Wrong block type (DLRR).
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// BT: 5 (DLRR), Reserved: 0, Block length: 2
			0x05, 0x00, 0x00, 0x02,
			0x12, 0x34, 0x56, 0x78,
			0x23, 0x45, 0x67, 0x89
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RrtExtendedReport::Parse(buffer3, sizeof(buffer3)));

		// Block length field announces more bytes than the buffer has.
		// clang-format off
		alignas(4) uint8_t buffer4[] =
		{
			// BT: 4 (RRT), Reserved: 0, Block length: 2
			0x04, 0x00, 0x00, 0x02,
			0x12, 0x34, 0x56, 0x78
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::RrtExtendedReport::Parse(buffer4, sizeof(buffer4)));
	}

	SECTION("RrtExtendedReport::Factory() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			// BT: 4 (RRT), Reserved: 0, Block length: 2
			0x04, 0x00, 0x00, 0x02,
			// NTP timestamp, most significant word: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// NTP timestamp, least significant word: 0x23456789
			0x23, 0x45, 0x67, 0x89
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::RrtExtendedReport> extendedReport{
			RTC::NEW_RTCP::RrtExtendedReport::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 12,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::RRT,
		  /*unknownType*/ false);

		REQUIRE(extendedReport->GetNtp().seconds == 0);
		REQUIRE(extendedReport->GetNtp().fractions == 0);

		/* Modify it. */

		extendedReport->SetNtp({ .seconds = 0x12345678, .fractions = 0x23456789 });

		REQUIRE(extendedReport->GetNtp().seconds == 0x12345678);
		REQUIRE(extendedReport->GetNtp().fractions == 0x23456789);
		REQUIRE(
		  helpers::areBuffersEqual(
		    extendedReport->GetBuffer(),
		    extendedReport->GetLength(),
		    expectedBuffer,
		    sizeof(expectedBuffer)) == true);
	}

	SECTION("RrtExtendedReport::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::NEW_RTCP::RrtExtendedReport::Factory(rtcpCommon::FactoryBuffer, 11), MediaSoupTypeError);
	}
}
