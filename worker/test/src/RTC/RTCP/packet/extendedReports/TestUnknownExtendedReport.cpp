#include "common.hpp"
#include "RTC/RTCP/packet/ExtendedReport.hpp"
#include "RTC/RTCP/packet/extendedReports/UnknownExtendedReport.hpp"
#include "test/include/RTC/RTCP/rtcpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP XR Unknown Extended Report", "[serializable][rtcp][extendedreport]")
{
	rtcpCommon::ResetBuffers();

	SECTION("UnknownExtendedReport::Parse() succeeds")
	{
		const auto checkExtendedReport =
		  [](const RTC::RTCP::UnknownExtendedReport* extendedReport) -> void
		{
			REQUIRE(extendedReport->HasUnknownValue() == true);
			REQUIRE(extendedReport->GetUnknownValueLength() == 8);
			REQUIRE(extendedReport->GetUnknownValue()[0] == 0x11);
			REQUIRE(extendedReport->GetUnknownValue()[1] == 0x22);
			REQUIRE(extendedReport->GetUnknownValue()[7] == 0x88);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// BT: 42, Type-specific: 0xAB, Block length: 2
			0x2A, 0xAB, 0x00, 0x02,
			// 8 bytes of value
			0x11, 0x22, 0x33, 0x44,
			0x55, 0x66, 0x77, 0x88
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::UnknownExtendedReport> extendedReport{
			RTC::RTCP::UnknownExtendedReport::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*blockType*/ static_cast<RTC::RTCP::ExtendedReport::BlockType>(42),
		  /*unknownType*/ true);

		checkExtendedReport(extendedReport.get());

		/* Serialize it. */

		extendedReport->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 12,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*blockType*/ static_cast<RTC::RTCP::ExtendedReport::BlockType>(42),
		  /*unknownType*/ true);

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
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*blockType*/ static_cast<RTC::RTCP::ExtendedReport::BlockType>(42),
		  /*unknownType*/ true);

		checkExtendedReport(extendedReport.get());
	}

	SECTION("UnknownExtendedReport::Parse() without value succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// BT: 99, Type-specific: 0, Block length: 0
			0x63, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::RTCP::UnknownExtendedReport> extendedReport{
			RTC::RTCP::UnknownExtendedReport::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
		  /*blockType*/ static_cast<RTC::RTCP::ExtendedReport::BlockType>(99),
		  /*unknownType*/ true);

		REQUIRE(extendedReport->HasUnknownValue() == false);
		REQUIRE(extendedReport->GetUnknownValueLength() == 0);
		REQUIRE(extendedReport->GetUnknownValue() == nullptr);
	}

	SECTION("UnknownExtendedReport::Parse() fails")
	{
		// Less bytes than the Extended Report block header.
		// clang-format off
		alignas(4) uint8_t buffer1[] =
		{
			0x2A, 0x00, 0x00
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::UnknownExtendedReport::Parse(buffer1, sizeof(buffer1)));

		// Block length field announces more bytes than the buffer has.
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// BT: 42, Type-specific: 0, Block length: 2
			0x2A, 0x00, 0x00, 0x02,
			// 4 bytes of value
			0x11, 0x22, 0x33, 0x44
		};
		// clang-format on

		REQUIRE(!RTC::RTCP::UnknownExtendedReport::Parse(buffer2, sizeof(buffer2)));
	}
}
