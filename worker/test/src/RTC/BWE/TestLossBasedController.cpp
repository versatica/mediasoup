#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/LossBasedController.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("BWE LossBasedController", "[bwe][lossbasedcontroller]")
{
	constexpr int64_t ObservationDurationLowerBoundUs{ 250 * 1000 };
	constexpr int64_t PacketSizeBytes{ 15000 };

	// Options that make the controller usable after a single observation, so
	// that a case doesn't have to feed a whole window to prove anything.
	const RTC::BWE::LossBasedController::LossBasedControllerOptions shortObservationOptions{
		.observationWindowSize = 2, .minNumObservations = 1
	};

	// Sequence number handed out to the packets of every case, which must not
	// repeat within a same observation.
	int64_t sequenceNumber{ 0 };

	// Results of a feedback covering the shortest span that closes an
	// observation, with the given number of its packets reported as lost.
	const auto createResults = [&sequenceNumber](
	                             int64_t firstSendTimeUs,
	                             size_t numPackets,
	                             size_t numLostPackets) -> std::vector<RTC::BWE::Types::PacketResult>
	{
		std::vector<RTC::BWE::Types::PacketResult> packetResults(numPackets);

		for (size_t idx{ 0 }; idx < numPackets; ++idx)
		{
			auto& packetResult = packetResults[idx];

			packetResult.sentPacket.sequenceNumber = sequenceNumber++;
			packetResult.sentPacket.size           = PacketSizeBytes;
			packetResult.sentPacket.sendTimeUs =
			  firstSendTimeUs + (static_cast<int64_t>(idx) * ObservationDurationLowerBoundUs);

			// The lost ones are the last ones, so that the span of send times is
			// the same no matter how many there are.
			if (idx < numPackets - numLostPackets)
			{
				packetResult.receiveTimeUs =
				  firstSendTimeUs + (static_cast<int64_t>(idx + 1) * ObservationDurationLowerBoundUs);
			}
		}

		return packetResults;
	};

	// Results of a feedback whose packets are all reported as lost, sent close to
	// each other so that they amount to a burst rather than to a steady rate.
	const auto createLostResults = [&sequenceNumber](
	                                 int64_t firstSendTimeUs,
	                                 size_t numPackets) -> std::vector<RTC::BWE::Types::PacketResult>
	{
		std::vector<RTC::BWE::Types::PacketResult> packetResults(numPackets);

		for (size_t idx{ 0 }; idx < numPackets; ++idx)
		{
			auto& packetResult = packetResults[idx];

			packetResult.sentPacket.sequenceNumber = sequenceNumber++;
			packetResult.sentPacket.size           = PacketSizeBytes;
			// The last one is what closes the observation, while the rest go right
			// after each other, which is what makes this a burst.
			packetResult.sentPacket.sendTimeUs =
			  idx == numPackets - 1 ? firstSendTimeUs + ObservationDurationLowerBoundUs
				                      : firstSendTimeUs + (static_cast<int64_t>(idx) * 10 * 1000);
		}

		return packetResults;
	};

	SECTION("there is no estimate until one is set to start from")
	{
		RTC::BWE::LossBasedController lossBasedController;

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(!lossBasedController.IsReady());
		REQUIRE(lossBasedController.GetResult().bitrate == RTC::BWE::Types::BitrateInfinite);
	}

	SECTION("there is no estimate while no feedback covers a long enough span")
	{
		RTC::BWE::LossBasedController lossBasedController;

		lossBasedController.SetBitrateEstimate(600000);

		REQUIRE(!lossBasedController.IsReady());

		// Half the shortest span an observation may cover.
		std::vector<RTC::BWE::Types::PacketResult> packetResults(2);

		for (size_t idx{ 0 }; idx < packetResults.size(); ++idx)
		{
			packetResults[idx].sentPacket.sequenceNumber = sequenceNumber++;
			packetResults[idx].sentPacket.size           = PacketSizeBytes;
			packetResults[idx].sentPacket.sendTimeUs =
			  static_cast<int64_t>(idx) * (ObservationDurationLowerBoundUs / 2);
			packetResults[idx].receiveTimeUs =
			  static_cast<int64_t>(idx + 1) * (ObservationDurationLowerBoundUs / 2);
		}

		lossBasedController.UpdateBitrateEstimate(
		  packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		REQUIRE(!lossBasedController.IsReady());
	}

	SECTION("the bitrate set is what the estimate starts from")
	{
		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateEstimate(600000);

		REQUIRE(!lossBasedController.IsReady());
		REQUIRE(lossBasedController.GetResult().bitrate == RTC::BWE::Types::BitrateInfinite);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		// Of the three candidates, the one above the current estimate is the one
		// predicting the least loss, so with no loss observed it's the most likely
		// one and nothing bounds it: 600000 * 1.02.
		REQUIRE(lossBasedController.IsReady());
		REQUIRE(lossBasedController.GetResult().bitrate == 612000);
	}

	SECTION("losing every packet brings the estimate down")
	{
		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateEstimate(600000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 2),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.IsReady());
		REQUIRE(lossBasedController.GetResult().bitrate < 600000);
		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);
	}

	SECTION("the estimate doesn't go above the maximum bitrate")
	{
		constexpr int64_t MaxBitrate{ 700000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateLimits(/*minBitrate*/ 0, MaxBitrate);
		lossBasedController.SetBitrateEstimate(600000);

		int64_t firstSendTimeUs{ 0 };

		// Feed feedback telling no loss at all for a while, which is what makes
		// the estimate grow.
		for (int i{ 0 }; i < 10; ++i)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(firstSendTimeUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
			  RTC::BWE::Types::BitrateInfinite,
			  /*inAlr*/ false);

			firstSendTimeUs += 2 * ObservationDurationLowerBoundUs;
		}

		REQUIRE(lossBasedController.GetResult().bitrate <= MaxBitrate);
	}

	SECTION("the estimate doesn't go below what the link is known to deliver")
	{
		constexpr int64_t AcknowledgedBitrate{ 500000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetAcknowledgedBitrate(AcknowledgedBitrate);
		lossBasedController.SetBitrateEstimate(600000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 2),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate >= AcknowledgedBitrate);
	}

	SECTION("the delay based estimate bounds this one")
	{
		constexpr int64_t DelayBasedEstimate{ 400000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateEstimate(600000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate <= DelayBasedEstimate);
	}

	SECTION("while not filling the link the estimate doesn't fall to what it delivers")
	{
		constexpr int64_t AcknowledgedBitrate{ 100000 };
		constexpr int64_t InitialBitrate{ 600000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateLimits(/*minBitrate*/ 10000, /*maxBitrate*/ 1000000000);
		lossBasedController.SetBitrateEstimate(InitialBitrate);
		lossBasedController.SetAcknowledgedBitrate(AcknowledgedBitrate);

		lossBasedController.UpdateBitrateEstimate(
		  createLostResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2),
		  /*delayBasedEstimate*/ 5000000,
		  /*inAlr*/ true);

		// In an application limited region what the link delivers says nothing
		// about what it could deliver, so it's no candidate to fall back to.
		REQUIRE(lossBasedController.GetResult().bitrate > AcknowledgedBitrate);
		REQUIRE(lossBasedController.GetResult().bitrate < InitialBitrate);
	}

	SECTION("while filling the link the estimate falls to what it delivers")
	{
		constexpr int64_t AcknowledgedBitrate{ 100000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateLimits(/*minBitrate*/ 10000, /*maxBitrate*/ 1000000000);
		lossBasedController.SetBitrateEstimate(600000);
		lossBasedController.SetAcknowledgedBitrate(AcknowledgedBitrate);

		lossBasedController.UpdateBitrateEstimate(
		  createLostResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2),
		  /*delayBasedEstimate*/ 5000000,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate == AcknowledgedBitrate);
	}

	SECTION("after backing off, recovering grows the estimate by the hold factor")
	{
		constexpr int64_t DelayBasedEstimate{ 5000000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateEstimate(600000);
		lossBasedController.SetAcknowledgedBitrate(100000);

		// Lose everything, which is what puts it in a loss limited state.
		lossBasedController.UpdateBitrateEstimate(
		  createLostResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		const int64_t bitrateAtLoss = lossBasedController.GetResult().bitrate;

		// Let the link recover until the estimate starts growing again.
		int64_t feedbackCount{ 2 };

		while (lossBasedController.GetResult().state != RTC::BWE::LossBasedController::State::INCREASING)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(
			    feedbackCount * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
			  DelayBasedEstimate,
			  /*inAlr*/ false);

			++feedbackCount;
		}

		// Growing out of a hold is bounded by the more conservative of the two
		// rampup factors, which is 1.2.
		REQUIRE(lossBasedController.GetResult().bitrate == static_cast<int64_t>(bitrateAtLoss * 1.2));
	}

	SECTION("a single observation of loss among good ones is taken as a spike")
	{
		constexpr int64_t DelayBasedEstimate{ 500000 };

		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{ .observationWindowSize =
		                                                                           5 };

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(DelayBasedEstimate);

		// Fill the window with observations telling no loss.
		for (int64_t i{ 0 }; i < 5; ++i)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(i * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
			  DelayBasedEstimate,
			  /*inAlr*/ false);
		}

		lossBasedController.UpdateBitrateEstimate(
		  createLostResults(5 * ObservationDurationLowerBoundUs, /*numPackets*/ 2),
		  DelayBasedEstimate,
		  /*inAlr*/ false);
		lossBasedController.UpdateBitrateEstimate(
		  createResults(6 * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		// The worst observation of the window is left out of the average, so that
		// single one doesn't count as the link being loss limited.
		REQUIRE(
		  lossBasedController.GetResult().state ==
		  RTC::BWE::LossBasedController::State::DELAY_BASED_ESTIMATE);
		REQUIRE(lossBasedController.GetResult().bitrate == DelayBasedEstimate);

		// A second one is not a spike anymore.
		lossBasedController.UpdateBitrateEstimate(
		  createLostResults(7 * ObservationDurationLowerBoundUs, /*numPackets*/ 2),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);
		REQUIRE(lossBasedController.GetResult().bitrate < DelayBasedEstimate);
	}

	SECTION("a loss above the preference threshold takes the estimate down")
	{
		// With the threshold below the loss that is about to be observed, the bias
		// towards higher bitrates is gone and the lower candidate wins.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.lossThresholdOfHighBitratePreference = 0.05, .observationWindowSize = 2, .minNumObservations = 1
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(600000);

		for (int64_t i{ 0 }; i < 2; ++i)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(i * ObservationDurationLowerBoundUs, /*numPackets*/ 10, /*numLostPackets*/ 1),
			  /*delayBasedEstimate*/ 5000000,
			  /*inAlr*/ false);
		}

		REQUIRE(lossBasedController.GetResult().bitrate < 600000);
	}

	SECTION("the estimate doesn't grow while the loss observed is worse than the inherent one")
	{
		// A single candidate above the current estimate, so that the only thing
		// that can hold the estimate back is the guard being tested.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.candidateFactors = { 1.2 }, .observationWindowSize = 2, .minNumObservations = 1
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(600000);

		for (int64_t i{ 0 }; i < 2; ++i)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(i * ObservationDurationLowerBoundUs, /*numPackets*/ 10, /*numLostPackets*/ 1),
			  RTC::BWE::Types::BitrateInfinite,
			  /*inAlr*/ false);
		}

		REQUIRE(lossBasedController.GetResult().bitrate == 600000);
	}

	SECTION("of two equally likely candidates the higher one is taken")
	{
		constexpr int64_t InitialBitrate{ 1000000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateEstimate(InitialBitrate);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  InitialBitrate,
		  /*inAlr*/ false);

		// Just above the candidate that the factor of 1.02 builds, so that it's the
		// delay based estimate the one added as a candidate and taken.
		constexpr int64_t DelayBasedEstimate{ 1020008 };

		lossBasedController.UpdateBitrateEstimate(
		  createResults(ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate == DelayBasedEstimate);
	}

	SECTION("a packet reported lost and received later doesn't take the estimate down")
	{
		constexpr int64_t InitialBitrate{ 2500000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateEstimate(InitialBitrate);

		// The second packet is reported as lost, which may just mean that it was
		// reordered and hasn't arrived yet.
		std::vector<RTC::BWE::Types::PacketResult> firstResults(3);

		for (size_t idx{ 0 }; idx < firstResults.size(); ++idx)
		{
			firstResults[idx].sentPacket.sequenceNumber = static_cast<int64_t>(idx) + 1;
			firstResults[idx].sentPacket.size           = PacketSizeBytes;
			firstResults[idx].sentPacket.sendTimeUs     = 0;

			if (idx != 1)
			{
				firstResults[idx].receiveTimeUs = 10 * 1000;
			}
		}

		// The next feedback reports it as received, and closes the observation.
		std::vector<RTC::BWE::Types::PacketResult> secondResults(3);
		const std::array<int64_t, 3> sequenceNumbers{ 2, 4, 5 };

		for (size_t idx{ 0 }; idx < secondResults.size(); ++idx)
		{
			secondResults[idx].sentPacket.sequenceNumber = sequenceNumbers.at(idx);
			secondResults[idx].sentPacket.size           = PacketSizeBytes;
			secondResults[idx].sentPacket.sendTimeUs     = idx == 1 ? ObservationDurationLowerBoundUs : 0;
			secondResults[idx].receiveTimeUs = secondResults[idx].sentPacket.sendTimeUs + (10 * 1000);
		}

		lossBasedController.UpdateBitrateEstimate(
		  firstResults, /*delayBasedEstimate*/ InitialBitrate, /*inAlr*/ false);
		lossBasedController.UpdateBitrateEstimate(
		  secondResults, /*delayBasedEstimate*/ InitialBitrate, /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate == InitialBitrate);
	}

	SECTION("loss while sending much more than usual is not taken as a spike")
	{
		constexpr int64_t DelayBasedEstimate{ 500000 };

		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{ .observationWindowSize =
		                                                                           5 };

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(DelayBasedEstimate);

		for (int64_t i{ 0 }; i < 5; ++i)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(i * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
			  DelayBasedEstimate,
			  /*inAlr*/ false);
		}

		// Losing everything of a burst that more than doubles the usual sending
		// rate explains the loss on its own, so it's not filtered out.
		lossBasedController.UpdateBitrateEstimate(
		  createLostResults(5 * ObservationDurationLowerBoundUs, /*numPackets*/ 5),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);
		REQUIRE(lossBasedController.GetResult().bitrate <= DelayBasedEstimate);
	}
}
