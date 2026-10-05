#include "common.hpp"
#include "RTC/BWE/PacketLossTracker.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("BWE PacketLossTracker", "[bwe][packetlosstracker]")
{
	constexpr uint32_t Ssrc1{ 1111 };
	constexpr uint32_t Ssrc2{ 2222 };

	// What a report block says about a stream.
	struct ReportBlock
	{
		uint32_t ssrc;
		uint32_t lastSeq;
		int32_t totalLost;
	};

	RTC::BWE::PacketLossTracker packetLossTracker;

	// Feed a Receiver Report made of the given blocks.
	const auto receiveReceiverReport = [&packetLossTracker](const std::vector<ReportBlock>& blocks)
	{
		auto packet = std::make_unique<RTC::RTCP::ReceiverReportPacket>();

		for (const auto& block : blocks)
		{
			auto* report = new RTC::RTCP::ReceiverReport();

			report->SetSsrc(block.ssrc);
			report->SetLastSeq(block.lastSeq);
			report->SetTotalLost(block.totalLost);

			packet->AddReport(report);
		}

		return packetLossTracker.ReceiveReceiverReport(packet.get());
	};

	SECTION("the first report of a stream only takes down its totals")
	{
		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		REQUIRE_FALSE(loss.has_value());
	}

	SECTION("two reports of a stream give what was lost and expected in between")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1100, .totalLost = 15 }
    });

		REQUIRE(loss.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& lossValue = loss.value();

		REQUIRE(lossValue.expectedPackets == 100);
		REQUIRE(lossValue.lostPackets == 10);
	}

	SECTION("the blocks of a report are added together")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 },
		    { .ssrc = Ssrc2, .lastSeq = 500,  .totalLost = 0 }
    });

		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1100, .totalLost = 15 },
		    { .ssrc = Ssrc2, .lastSeq = 700,  .totalLost = 3  }
    });

		REQUIRE(loss.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& lossValue = loss.value();

		REQUIRE(lossValue.expectedPackets == 300);
		REQUIRE(lossValue.lostPackets == 13);
	}

	SECTION("a stream seen for the first time in a later report adds nothing")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		// The second stream arrives with totals of its own, which say nothing until
		// there is a later report to measure them against.
		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1100, .totalLost = 15  },
		    { .ssrc = Ssrc2, .lastSeq = 9000, .totalLost = 400 }
    });

		REQUIRE(loss.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& lossValue = loss.value();

		REQUIRE(lossValue.expectedPackets == 100);
		REQUIRE(lossValue.lostPackets == 10);
	}

	SECTION("the probation stream is left out")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1,                        .lastSeq = 1000, .totalLost = 5 },
		    { .ssrc = RTC::Consts::BweProbeRtpSsrc, .lastSeq = 100,  .totalLost = 0 }
    });

		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1,                        .lastSeq = 1100, .totalLost = 15  },
		    { .ssrc = RTC::Consts::BweProbeRtpSsrc, .lastSeq = 300,  .totalLost = 150 }
    });

		REQUIRE(loss.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& lossValue = loss.value();

		REQUIRE(lossValue.expectedPackets == 100);
		REQUIRE(lossValue.lostPackets == 10);
	}

	SECTION("duplicates make the lost count negative")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		// The running total of lost packets goes down when duplicates arrive.
		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1100, .totalLost = 3 }
    });

		REQUIRE(loss.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& lossValue = loss.value();

		REQUIRE(lossValue.expectedPackets == 100);
		REQUIRE(lossValue.lostPackets == -2);
	}

	SECTION("a report that arrives after a newer one of the same stream gives no value")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1100, .totalLost = 15 }
    });

		// RTCP packets may be reordered on the way, so a report built before the
		// previous one may still arrive after it. Its totals go backwards.
		const auto staleLoss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1050, .totalLost = 10 }
    });

		REQUIRE_FALSE(staleLoss.has_value());

		// And what it said is kept, so the next report is measured from there.
		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1150, .totalLost = 12 }
    });

		REQUIRE(loss.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& lossValue = loss.value();

		REQUIRE(lossValue.expectedPackets == 100);
		REQUIRE(lossValue.lostPackets == 2);
	}

	SECTION("a report that moves nothing gives no value")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		REQUIRE_FALSE(loss.has_value());
	}

	SECTION("a report where nothing got through gives no value")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1100, .totalLost = 105 }
    });

		REQUIRE_FALSE(loss.has_value());
	}

	SECTION("a removed stream is measured from scratch again")
	{
		receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1000, .totalLost = 5 }
    });

		packetLossTracker.RemoveStream(Ssrc1);

		// Its totals are gone, so this report is a first sighting again.
		const auto firstLoss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1100, .totalLost = 15 }
    });

		REQUIRE_FALSE(firstLoss.has_value());

		const auto loss = receiveReceiverReport(
		  {
		    { .ssrc = Ssrc1, .lastSeq = 1200, .totalLost = 20 }
    });

		REQUIRE(loss.has_value());

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& lossValue = loss.value();

		REQUIRE(lossValue.expectedPackets == 100);
		REQUIRE(lossValue.lostPackets == 5);
	}
}
