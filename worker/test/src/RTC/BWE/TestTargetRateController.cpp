#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/TargetRateController.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE TargetRateController", "[bwe][targetratecontroller]")
{
	constexpr int64_t InitialTimeUs{ 123456 * 1000 };
	// Shortest span of send times that closes an observation of the loss
	// controller, and hence the pace at which it can be fed.
	constexpr int64_t ObservationDurationLowerBoundUs{ 250 * 1000 };
	constexpr int64_t PacketSizeBytes{ 15000 };

	int64_t sequenceNumber{ 0 };

	// Results of a feedback closing one observation of the loss controller, with
	// every packet reported as lost or as received.
	const auto createResults = [&sequenceNumber](
	                             int64_t firstSendTimeUs,
	                             bool received) -> std::vector<RTC::BWE::Types::PacketResult>
	{
		std::vector<RTC::BWE::Types::PacketResult> packetResults(2);

		for (size_t idx{ 0 }; idx < packetResults.size(); ++idx)
		{
			auto& packetResult = packetResults[idx];

			packetResult.sentPacket.sequenceNumber = sequenceNumber++;
			packetResult.sentPacket.size           = PacketSizeBytes;
			packetResult.sentPacket.sendTimeUs =
			  firstSendTimeUs + (static_cast<int64_t>(idx) * ObservationDurationLowerBoundUs);

			if (received)
			{
				packetResult.receiveTimeUs =
				  firstSendTimeUs + (static_cast<int64_t>(idx + 1) * ObservationDurationLowerBoundUs);
			}
		}

		return packetResults;
	};

	// Feed a bound of the given kind and let the controller act on it.
	const auto feedLimit = [](
	                         RTC::BWE::TargetRateController& targetRateController,
	                         bool delayBased,
	                         int64_t bitrate,
	                         int64_t nowUs) -> void
	{
		if (delayBased)
		{
			targetRateController.SetDelayBasedEstimate(bitrate);
		}
		else
		{
			targetRateController.SetReceiverEstimate(bitrate);
		}

		targetRateController.Update(nowUs);
	};

	SECTION("the first bound applies at once but a later one has to wait")
	{
		// Both bounds behave the same here, so the very same case is run twice.
		for (const bool delayBased : { false, true })
		{
			RTC::BWE::TargetRateController targetRateController;
			int64_t nowUs{ InitialTimeUs };

			targetRateController.SetBitrateLimits(100000, 1500000);
			targetRateController.SetSendBitrate(200000);

			targetRateController.UpdatePacketsLost(/*lostPackets*/ 0, /*totalPackets*/ 1, nowUs);
			targetRateController.UpdateRtt(50 * 1000);

			constexpr int64_t FirstBitrate{ 1000000 };
			constexpr int64_t SecondBitrate{ FirstBitrate + 500000 };

			feedLimit(targetRateController, delayBased, FirstBitrate, nowUs);

			REQUIRE(targetRateController.GetTargetBitrate() == FirstBitrate);

			// Once the start phase is over the target no longer just follows its
			// bounds, so the second one only raises the ceiling.
			nowUs += 2001 * 1000;

			feedLimit(targetRateController, delayBased, SecondBitrate, nowUs);

			REQUIRE(targetRateController.GetTargetBitrate() == FirstBitrate);
		}
	}

	SECTION("a decrease is not reapplied while no further loss is reported")
	{
		constexpr int64_t MinBitrate{ 100000 };
		constexpr int64_t InitialBitrate{ 1000000 };
		constexpr uint8_t FractionLost{ 128 };
		constexpr int64_t RttUs{ 50 * 1000 };

		RTC::BWE::TargetRateController targetRateController;
		int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(MinBitrate, 1500000);
		targetRateController.SetSendBitrate(InitialBitrate);

		nowUs += 10000 * 1000;

		REQUIRE(targetRateController.GetTargetBitrate() == InitialBitrate);
		REQUIRE(targetRateController.GetFractionLost() == 0);
		REQUIRE(targetRateController.GetRttUs() == 0);

		// Report heavy loss so that the target goes down.
		targetRateController.UpdatePacketsLost(/*lostPackets*/ 50, /*totalPackets*/ 100, nowUs);
		targetRateController.UpdateRtt(RttUs);

		nowUs += 1000 * 1000;

		targetRateController.Update(nowUs);

		const int64_t decreasedBitrate = targetRateController.GetTargetBitrate();

		REQUIRE(decreasedBitrate < InitialBitrate);
		// A single decrease must not reach the minimum, or this case would prove
		// nothing.
		REQUIRE(decreasedBitrate > MinBitrate);
		REQUIRE(targetRateController.GetFractionLost() == FractionLost);
		REQUIRE(targetRateController.GetRttUs() == RttUs);

		// Without a new report there is nothing saying whether the decrease was
		// enough, so the target must not move again in either direction.
		nowUs += 1000 * 1000;

		targetRateController.Update(nowUs);

		REQUIRE(targetRateController.GetTargetBitrate() == decreasedBitrate);
		REQUIRE(targetRateController.GetFractionLost() == FractionLost);
		REQUIRE(targetRateController.GetRttUs() == RttUs);
	}

	SECTION("setting the send bitrate overrides the bound of the delay based path")
	{
		constexpr int64_t InitialBitrate{ 300000 };
		constexpr int64_t DelayBasedBitrate{ 350000 };
		constexpr int64_t ForcedHighBitrate{ 2500000 };

		RTC::BWE::TargetRateController targetRateController;
		const int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(10000, 10000000);
		targetRateController.SetSendBitrate(InitialBitrate);

		targetRateController.SetDelayBasedEstimate(DelayBasedBitrate);
		targetRateController.Update(nowUs);

		REQUIRE(targetRateController.GetTargetBitrate() >= InitialBitrate);
		REQUIRE(targetRateController.GetTargetBitrate() <= DelayBasedBitrate);

		targetRateController.SetSendBitrate(ForcedHighBitrate);

		REQUIRE(targetRateController.GetTargetBitrate() == ForcedHighBitrate);
	}

	SECTION("a negative number of lost packets doesn't overflow the reported fraction")
	{
		RTC::BWE::TargetRateController targetRateController;
		int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(100000, 1500000);
		targetRateController.SetSendBitrate(1000000);

		nowUs += 10000 * 1000;

		REQUIRE(targetRateController.GetFractionLost() == 0);

		targetRateController.UpdatePacketsLost(/*lostPackets*/ -1, /*totalPackets*/ 100, nowUs);

		REQUIRE(targetRateController.GetFractionLost() == 0);
	}

	SECTION("the round trip time is above the limit once it exceeds it")
	{
		RTC::BWE::TargetRateController targetRateController;
		const int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(10000, 10000000);
		targetRateController.SetSendBitrate(300000);

		targetRateController.UpdatePropagationRtt(/*propagationRttUs*/ 5000 * 1000, nowUs);

		REQUIRE(targetRateController.IsRttAboveLimit());
	}

	SECTION("the round trip time is below the limit while it doesn't reach it")
	{
		RTC::BWE::TargetRateController targetRateController;
		const int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(10000, 10000000);
		targetRateController.SetSendBitrate(300000);

		targetRateController.UpdatePropagationRtt(/*propagationRttUs*/ 1000 * 1000, nowUs);

		REQUIRE(!targetRateController.IsRttAboveLimit());
	}

	SECTION("the round trip time backoff can be disabled")
	{
		// A limit that no round trip time can reach disables the backoff, which is
		// what the sender of a link known to be slow would want.
		const RTC::BWE::TargetRateController::TargetRateControllerOptions options{
			.maxRttUs = RTC::BWE::Types::TimeUsInfinite
		};

		RTC::BWE::TargetRateController targetRateController(options);
		const int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(10000, 10000000);
		targetRateController.SetSendBitrate(300000);

		targetRateController.UpdatePropagationRtt(/*propagationRttUs*/ 5000 * 1000, nowUs);

		REQUIRE(!targetRateController.IsRttAboveLimit());
	}

	SECTION("a round trip time above the limit drops the target")
	{
		constexpr int64_t InitialBitrate{ 300000 };

		RTC::BWE::TargetRateController targetRateController;
		int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(10000, 10000000);
		targetRateController.SetSendBitrate(InitialBitrate);

		targetRateController.UpdatePropagationRtt(/*propagationRttUs*/ 5000 * 1000, nowUs);
		targetRateController.Update(nowUs);

		// Four fifths of 300000.
		REQUIRE(targetRateController.GetTargetBitrate() == 240000);

		// The drop happens at most once per interval.
		nowUs += 500 * 1000;

		targetRateController.Update(nowUs);

		REQUIRE(targetRateController.GetTargetBitrate() == 240000);

		nowUs += 500 * 1000;

		targetRateController.Update(nowUs);

		// And four fifths of that.
		REQUIRE(targetRateController.GetTargetBitrate() == 192000);
	}

	SECTION("a report telling no loss increases the target by 8% plus a bit")
	{
		constexpr int64_t InitialBitrate{ 300000 };

		RTC::BWE::TargetRateController targetRateController;
		int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(10000, 10000000);
		targetRateController.SetSendBitrate(InitialBitrate);

		// A report covering enough packets reconsiders the target by itself.
		targetRateController.UpdatePacketsLost(/*lostPackets*/ 0, /*totalPackets*/ 100, nowUs);

		// 8% over 300000 is 324000, plus the extra 1000.
		REQUIRE(targetRateController.GetTargetBitrate() == 325000);

		// Long enough for the value grown from to have left the window, so the next
		// increase grows from the current target.
		nowUs += 2001 * 1000;

		targetRateController.Update(nowUs);

		// And 8% over 325000 is 351000, plus the extra 1000.
		REQUIRE(targetRateController.GetTargetBitrate() == 352000);
	}

	SECTION("the loss rules drive the target until the loss controller is ready")
	{
		constexpr int64_t InitialBitrate{ 600000 };

		RTC::BWE::TargetRateController targetRateController;
		int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(10000, 10000000);
		targetRateController.SetSendBitrate(InitialBitrate);
		// NOTE: After SetSendBitrate(), which drops it on purpose, so that the loss
		// controller has a bitrate to start its estimate from.
		targetRateController.SetDelayBasedEstimate(InitialBitrate);

		// Two observations, one short of what the loss controller needs.
		for (int64_t i{ 0 }; i < 2; ++i)
		{
			targetRateController.UpdateLossBasedController(
			  createResults(nowUs, /*received*/ false), /*inAlr*/ false, nowUs);

			nowUs += 2 * ObservationDurationLowerBoundUs;
		}

		// Losing every packet would have brought the estimate down, so the target
		// staying put shows that nothing of it is being used yet.
		REQUIRE(
		  targetRateController.GetLossBasedState() ==
		  RTC::BWE::LossBasedController::State::DELAY_BASED_ESTIMATE);
		REQUIRE(targetRateController.GetTargetBitrate() == InitialBitrate);
	}

	SECTION("the loss controller takes the target over once it is ready")
	{
		constexpr int64_t InitialBitrate{ 600000 };

		RTC::BWE::TargetRateController targetRateController;
		int64_t nowUs{ InitialTimeUs };

		targetRateController.SetBitrateLimits(10000, 10000000);
		targetRateController.SetSendBitrate(InitialBitrate);
		// NOTE: After SetSendBitrate(), which drops it on purpose, so that the loss
		// controller has a bitrate to start its estimate from.
		targetRateController.SetDelayBasedEstimate(InitialBitrate);

		for (int64_t i{ 0 }; i < 3; ++i)
		{
			targetRateController.UpdateLossBasedController(
			  createResults(nowUs, /*received*/ false), /*inAlr*/ false, nowUs);

			nowUs += 2 * ObservationDurationLowerBoundUs;
		}

		REQUIRE(
		  targetRateController.GetLossBasedState() == RTC::BWE::LossBasedController::State::DECREASING);
		REQUIRE(targetRateController.GetTargetBitrate() < InitialBitrate);
	}
}
