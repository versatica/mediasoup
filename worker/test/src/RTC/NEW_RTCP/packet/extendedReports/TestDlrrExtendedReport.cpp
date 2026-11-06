#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/NEW_RTCP/packet/ExtendedReport.hpp"
#include "RTC/NEW_RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "test/include/RTC/NEW_RTCP/rtcpCommon.hpp"
#include "test/include/testHelpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstring> // std::memset()

SCENARIO("RTCP XR DLRR Extended Report (5)", "[serializable][rtcp][extendedreport]")
{
	rtcpCommon::ResetBuffers();

	SECTION("DlrrExtendedReport::Parse() succeeds")
	{
		const auto checkExtendedReport = [](const RTC::NEW_RTCP::DlrrExtendedReport* extendedReport) -> void
		{
			const auto subBlocks = extendedReport->GetSubBlocks();

			REQUIRE(extendedReport->GetNumberOfSubBlocks() == 2);
			REQUIRE(subBlocks.size() == 2);

			REQUIRE(subBlocks[0].ssrc == 0x11121314);
			REQUIRE(subBlocks[0].lastReceiverReport == 0x00110011);
			REQUIRE(subBlocks[0].delaySinceLastReceiverReport == 0x11001100);

			REQUIRE(subBlocks[1].ssrc == 0x21222324);
			REQUIRE(subBlocks[1].lastReceiverReport == 0x00220022);
			REQUIRE(subBlocks[1].delaySinceLastReceiverReport == 0x22002200);
		};

		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// BT: 5 (DLRR), Reserved: 0, Block length: 6
			0x05, 0x00, 0x00, 0x06,
			// SSRC 1: 0x11121314
			0x11, 0x12, 0x13, 0x14,
			// LRR 1: 0x00110011
			0x00, 0x11, 0x00, 0x11,
			// DLRR 1: 0x11001100
			0x11, 0x00, 0x11, 0x00,
			// SSRC 2: 0x21222324
			0x21, 0x22, 0x23, 0x24,
			// LRR 2: 0x00220022
			0x00, 0x22, 0x00, 0x22,
			// DLRR 2: 0x22002200
			0x22, 0x00, 0x22, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::DlrrExtendedReport> extendedReport{
			RTC::NEW_RTCP::DlrrExtendedReport::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 28,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR,
		  /*unknownType*/ false);

		checkExtendedReport(extendedReport.get());

		// No space to add more sub-blocks.
		REQUIRE_THROWS_AS(
		  extendedReport->AddSubBlock(
		    { .ssrc = 1, .lastReceiverReport = 2, .delaySinceLastReceiverReport = 3 }),
		  MediaSoupTypeError);

		checkExtendedReport(extendedReport.get());

		/* Serialize it. */

		extendedReport->Serialize(rtcpCommon::SerializeBuffer, sizeof(rtcpCommon::SerializeBuffer));

		std::memset(buffer, 0xFF, sizeof(buffer));

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ rtcpCommon::SerializeBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::SerializeBuffer),
		  /*length*/ 28,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR,
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
		  /*length*/ 28,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR,
		  /*unknownType*/ false);

		checkExtendedReport(extendedReport.get());
	}

	SECTION("DlrrExtendedReport::Parse() with one sub-block succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// BT: 5 (DLRR), Reserved: 0, Block length: 3
			0x05, 0x00, 0x00, 0x03,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// LRR: 0x23344556
			0x23, 0x34, 0x45, 0x56,
			// DLRR: 0x33343536
			0x33, 0x34, 0x35, 0x36
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::DlrrExtendedReport> extendedReport{
			RTC::NEW_RTCP::DlrrExtendedReport::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 16,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR,
		  /*unknownType*/ false);

		const auto subBlocks = extendedReport->GetSubBlocks();

		REQUIRE(extendedReport->GetNumberOfSubBlocks() == 1);
		REQUIRE(subBlocks.size() == 1);
		REQUIRE(subBlocks[0].ssrc == 0x12345678);
		REQUIRE(subBlocks[0].lastReceiverReport == 0x23344556);
		REQUIRE(subBlocks[0].delaySinceLastReceiverReport == 0x33343536);
	}

	SECTION("DlrrExtendedReport::Parse() without sub-blocks succeeds")
	{
		// clang-format off
		alignas(4) uint8_t buffer[] =
		{
			// BT: 5 (DLRR), Reserved: 0, Block length: 0
			0x05, 0x00, 0x00, 0x00
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::DlrrExtendedReport> extendedReport{
			RTC::NEW_RTCP::DlrrExtendedReport::Parse(buffer, sizeof(buffer))
		};

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ buffer,
		  /*bufferLength*/ sizeof(buffer),
		  /*length*/ 4,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR,
		  /*unknownType*/ false);

		REQUIRE(extendedReport->GetNumberOfSubBlocks() == 0);
		REQUIRE(extendedReport->GetSubBlocks().empty());
	}

	SECTION("DlrrExtendedReport::Parse() fails")
	{
		// Parse succeeds only when the block length (in 32-bit words, excluding
		// the header) is multiple of 3.
		alignas(4) uint8_t buffer1[256]{};

		buffer1[0] = 0x05;

		for (uint8_t blockLength{ 3 }; blockLength < 6; ++blockLength)
		{
			buffer1[3] = blockLength;

			const std::unique_ptr<RTC::NEW_RTCP::DlrrExtendedReport> extendedReport{
				RTC::NEW_RTCP::DlrrExtendedReport::Parse(buffer1, sizeof(buffer1))
			};

			REQUIRE((extendedReport != nullptr) == (blockLength % 3 == 0));
		}

		// Wrong block type (RRT).
		// clang-format off
		alignas(4) uint8_t buffer2[] =
		{
			// BT: 4 (RRT), Reserved: 0, Block length: 3
			0x04, 0x00, 0x00, 0x03,
			0x12, 0x34, 0x56, 0x78,
			0x23, 0x34, 0x45, 0x56,
			0x33, 0x34, 0x35, 0x36
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::DlrrExtendedReport::Parse(buffer2, sizeof(buffer2)));

		// Block length field announces more bytes than the buffer has.
		// clang-format off
		alignas(4) uint8_t buffer3[] =
		{
			// BT: 5 (DLRR), Reserved: 0, Block length: 3
			0x05, 0x00, 0x00, 0x03,
			0x12, 0x34, 0x56, 0x78,
			0x23, 0x34, 0x45, 0x56
		};
		// clang-format on

		REQUIRE(!RTC::NEW_RTCP::DlrrExtendedReport::Parse(buffer3, sizeof(buffer3)));
	}

	SECTION("DlrrExtendedReport::Factory() succeeds")
	{
		// clang-format off
		alignas(4) uint8_t expectedBuffer[] =
		{
			// BT: 5 (DLRR), Reserved: 0, Block length: 3
			0x05, 0x00, 0x00, 0x03,
			// SSRC: 0x12345678
			0x12, 0x34, 0x56, 0x78,
			// LRR: 0x23344556
			0x23, 0x34, 0x45, 0x56,
			// DLRR: 0x33343536
			0x33, 0x34, 0x35, 0x36
		};
		// clang-format on

		std::unique_ptr<RTC::NEW_RTCP::DlrrExtendedReport> extendedReport{
			RTC::NEW_RTCP::DlrrExtendedReport::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 4,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR,
		  /*unknownType*/ false);

		REQUIRE(extendedReport->GetNumberOfSubBlocks() == 0);
		REQUIRE(extendedReport->GetSubBlocks().empty());

		/* Modify it. */

		extendedReport->AddSubBlock(
		  { .ssrc                         = 0x12345678,
			  .lastReceiverReport           = 0x23344556,
			  .delaySinceLastReceiverReport = 0x33343536 });

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 16,
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR,
		  /*unknownType*/ false);

		REQUIRE(
		  helpers::areBuffersEqual(
		    extendedReport->GetBuffer(),
		    extendedReport->GetLength(),
		    expectedBuffer,
		    sizeof(expectedBuffer)) == true);

		const auto subBlocks = extendedReport->GetSubBlocks();

		REQUIRE(extendedReport->GetNumberOfSubBlocks() == 1);
		REQUIRE(subBlocks.size() == 1);
		REQUIRE(subBlocks[0].ssrc == 0x12345678);
		REQUIRE(subBlocks[0].lastReceiverReport == 0x23344556);
		REQUIRE(subBlocks[0].delaySinceLastReceiverReport == 0x33343536);
	}

	SECTION("DlrrExtendedReport with many sub-blocks")
	{
		constexpr size_t NumberOfSubBlocks{ 50 };

		std::unique_ptr<RTC::NEW_RTCP::DlrrExtendedReport> extendedReport{
			RTC::NEW_RTCP::DlrrExtendedReport::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		for (uint32_t idx{ 1 }; idx <= NumberOfSubBlocks; ++idx)
		{
			extendedReport->AddSubBlock(
			  { .ssrc                         = 0x12345678 + idx,
				  .lastReceiverReport           = 0x23344556 + idx,
				  .delaySinceLastReceiverReport = 0x33343536 + idx });
		}

		CHECK_RTCP_EXTENDED_REPORT(
		  /*extendedReport*/ extendedReport.get(),
		  /*buffer*/ rtcpCommon::FactoryBuffer,
		  /*bufferLength*/ sizeof(rtcpCommon::FactoryBuffer),
		  /*length*/ 4 + (NumberOfSubBlocks * 12),
		  /*blockType*/ RTC::NEW_RTCP::ExtendedReport::BlockType::DLRR,
		  /*unknownType*/ false);

		/* Parse it. */

		const std::unique_ptr<RTC::NEW_RTCP::DlrrExtendedReport> parsedExtendedReport{
			RTC::NEW_RTCP::DlrrExtendedReport::Parse(
			  extendedReport->GetBuffer(), extendedReport->GetLength())
		};

		REQUIRE(parsedExtendedReport);
		REQUIRE(parsedExtendedReport->GetLength() == 4 + (NumberOfSubBlocks * 12));
		REQUIRE(parsedExtendedReport->GetNumberOfSubBlocks() == NumberOfSubBlocks);

		const auto subBlocks = parsedExtendedReport->GetSubBlocks();

		REQUIRE(subBlocks.size() == NumberOfSubBlocks);
		REQUIRE(subBlocks[0].ssrc == 0x12345679);
		REQUIRE(subBlocks[49].ssrc == 0x123456AA);
		REQUIRE(subBlocks[49].lastReceiverReport == 0x23344588);
		REQUIRE(subBlocks[49].delaySinceLastReceiverReport == 0x33343568);
	}

	SECTION("DlrrExtendedReport::Factory() throws if the buffer is too small")
	{
		REQUIRE_THROWS_AS(
		  RTC::NEW_RTCP::DlrrExtendedReport::Factory(rtcpCommon::FactoryBuffer, 3), MediaSoupTypeError);
	}

	SECTION("DlrrExtendedReport::ComputeLengthFor() succeeds")
	{
		REQUIRE(RTC::NEW_RTCP::DlrrExtendedReport::ComputeLengthFor(0) == 4);

		const std::unique_ptr<RTC::NEW_RTCP::DlrrExtendedReport> extendedReport{
			RTC::NEW_RTCP::DlrrExtendedReport::Factory(
			  rtcpCommon::FactoryBuffer, sizeof(rtcpCommon::FactoryBuffer))
		};

		extendedReport->AddSubBlock(
		  { .ssrc = 1, .lastReceiverReport = 1001, .delaySinceLastReceiverReport = 2001 });
		extendedReport->AddSubBlock(
		  { .ssrc = 2, .lastReceiverReport = 1002, .delaySinceLastReceiverReport = 2002 });

		REQUIRE(RTC::NEW_RTCP::DlrrExtendedReport::ComputeLengthFor(2) == 28);
		REQUIRE(extendedReport->GetLength() == RTC::NEW_RTCP::DlrrExtendedReport::ComputeLengthFor(2));
	}
}
