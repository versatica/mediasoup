#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/LossBasedController.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath> // std::llround()
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
		//
		// NOTE: Bounded so that a controller that never gets there fails the case
		// instead of hanging.
		int64_t feedbackCount{ 2 };

		while (lossBasedController.GetResult().state != RTC::BWE::LossBasedController::State::INCREASING &&
		       feedbackCount < 100)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(
			    feedbackCount * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
			  DelayBasedEstimate,
			  /*inAlr*/ false);

			++feedbackCount;
		}

		// Growing out of a hold is bounded by the more conservative of the two
		// rampup factors, which is 1.2. The bitrate backed off to depends on the
		// whole run, so the factor is applied to it here, rounded as every bitrate
		// derived from another one is.
		REQUIRE(
		  lossBasedController.GetResult().bitrate ==
		  std::llround(static_cast<double>(bitrateAtLoss) * 1.2));
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

	SECTION("a loss below the preference threshold still lets the estimate grow")
	{
		// The guard that holds the estimate back while the observed loss is worse
		// than the inherent one is disabled, so that nothing but the preference
		// decides.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.lossThresholdOfHighBitratePreference         = 0.20,
			.observationWindowSize                        = 2,
			.minNumObservations                           = 1,
			.notIncreaseIfInherentLossLessThanAverageLoss = false
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

		REQUIRE(lossBasedController.GetResult().bitrate > 600000);
	}

	SECTION("a feedback that closes no observation changes nothing")
	{
		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateEstimate(600000);
		lossBasedController.SetAcknowledgedBitrate(300000);

		const auto packetResults =
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0);

		lossBasedController.UpdateBitrateEstimate(
		  packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		const int64_t bitrate = lossBasedController.GetResult().bitrate;

		// The very same feedback again covers no new span of send times.
		lossBasedController.UpdateBitrateEstimate(
		  packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate == bitrate);
	}

	SECTION("right after a decrease the estimate is bounded for a while")
	{
		constexpr int64_t DelayBasedEstimate{ 5000000 };
		constexpr int64_t DelayedIncreaseWindowUs{ 300 * 1000 };

		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.observationWindowSize = 15, .minNumObservations = 1
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(600000);
		lossBasedController.SetAcknowledgedBitrate(300000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		// Raise what the link is known to deliver, so that it isn't what keeps the
		// estimate low.
		lossBasedController.SetAcknowledgedBitrate(5000000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(DelayedIncreaseWindowUs - (2 * 1000), /*numPackets*/ 2, /*numLostPackets*/ 1),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		const int64_t bitrateAfterDecrease = lossBasedController.GetResult().bitrate;

		// Still within the window that follows the decrease.
		lossBasedController.UpdateBitrateEstimate(
		  createResults(DelayedIncreaseWindowUs - (1 * 1000), /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate == bitrateAfterDecrease);
	}

	SECTION("once that window is over the estimate grows again")
	{
		constexpr int64_t DelayBasedEstimate{ 5000000 };
		constexpr int64_t DelayedIncreaseWindowUs{ 300 * 1000 };

		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.observationWindowSize = 15, .minNumObservations = 1
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(600000);
		lossBasedController.SetAcknowledgedBitrate(300000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		lossBasedController.SetAcknowledgedBitrate(5000000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(DelayedIncreaseWindowUs - (1 * 1000), /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		const int64_t bitrateWithinWindow = lossBasedController.GetResult().bitrate;

		lossBasedController.UpdateBitrateEstimate(
		  createResults(DelayedIncreaseWindowUs + (1 * 1000), /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate >= bitrateWithinWindow);
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

	SECTION("the state says the delay based path rules once the estimate reaches the maximum")
	{
		constexpr int64_t MaxBitrate{ 1000000 };
		constexpr int64_t DelayBasedEstimate{ 2000000 };

		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateLimits(/*minBitrate*/ 10000, MaxBitrate);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		REQUIRE(
		  lossBasedController.GetResult().state ==
		  RTC::BWE::LossBasedController::State::DELAY_BASED_ESTIMATE);
		REQUIRE(lossBasedController.GetResult().bitrate == MaxBitrate);

		// Half of the packets lost takes it off the maximum and out of that state.
		lossBasedController.UpdateBitrateEstimate(
		  createResults(ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 1),
		  DelayBasedEstimate,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);
		REQUIRE(lossBasedController.GetResult().bitrate < MaxBitrate);

		// And it comes back to it once the link stops losing packets.
		int64_t feedbackCount{ 2 };

		// NOTE: Bounded so that a controller that never gets there fails the case
		// instead of hanging.
		while (lossBasedController.GetResult().state !=
		         RTC::BWE::LossBasedController::State::DELAY_BASED_ESTIMATE &&
		       feedbackCount < 100)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(
			    feedbackCount * ObservationDurationLowerBoundUs,
			    /*numPackets*/ 2,
			    /*numLostPackets*/ 0),
			  DelayBasedEstimate,
			  /*inAlr*/ false);

			++feedbackCount;
		}

		REQUIRE(lossBasedController.GetResult().bitrate == MaxBitrate);
	}

	SECTION("while holding, the bitrate held is never below what the link delivers")
	{
		constexpr int64_t AcknowledgedBitrate{ 1000000 };

		// A hold long enough to still be in place on the next observation.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.observationWindowSize       = 2,
			.minNumObservations          = 1,
			.lowerBoundByAckedRateFactor = 1.0,
			.holdDurationFactor          = 10.0
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(2500000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 1),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);

		lossBasedController.SetAcknowledgedBitrate(AcknowledgedBitrate);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 1),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);
		REQUIRE(lossBasedController.GetResult().bitrate == AcknowledgedBitrate);
	}

	SECTION("a hold ends as soon as the delay based path works again")
	{
		RTC::BWE::LossBasedController lossBasedController(shortObservationOptions);

		lossBasedController.SetBitrateEstimate(2500000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 1),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);

		const int64_t heldBitrate        = lossBasedController.GetResult().bitrate;
		const int64_t delayBasedEstimate = heldBitrate + 10000;

		for (int64_t i{ 1 }; i <= 2; ++i)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(i * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
			  delayBasedEstimate,
			  /*inAlr*/ false);
		}

		REQUIRE(
		  lossBasedController.GetResult().state ==
		  RTC::BWE::LossBasedController::State::DELAY_BASED_ESTIMATE);
		REQUIRE(lossBasedController.GetResult().bitrate == delayBasedEstimate);
	}

	SECTION("the estimate grows even while what the link delivers is bounding it")
	{
		// The preference for a high bitrate stays on no matter the loss, and no
		// immediate upper bound gets in the way, so that only the bound by the
		// acknowledged bitrate is left.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.bitrateRampupUpperBoundFactor        = 1.2,
			.lossThresholdOfHighBitratePreference = 0.99,
			.observationWindowSize                = 2,
			.minNumObservations                   = 1,
			.immediateUpperBoundBitrateBalance    = 10000000
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(600000);
		lossBasedController.SetAcknowledgedBitrate(300000);

		lossBasedController.UpdateBitrateEstimate(
		  createLostResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2),
		  /*delayBasedEstimate*/ 5000000,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);

		const int64_t bitrateAfterLoss = lossBasedController.GetResult().bitrate;

		REQUIRE(bitrateAfterLoss < 600000);

		// What the link delivers is now half of the estimate, so the bound by it
		// would hold the estimate still.
		lossBasedController.SetAcknowledgedBitrate(bitrateAfterLoss / 2);

		int64_t feedbackCount{ 1 };

		// NOTE: Bounded so that a controller that never gets there fails the case
		// instead of hanging.
		while (lossBasedController.GetResult().state != RTC::BWE::LossBasedController::State::INCREASING &&
		       feedbackCount < 100)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(
			    feedbackCount * ObservationDurationLowerBoundUs,
			    /*numPackets*/ 2,
			    /*numLostPackets*/ 0),
			  /*delayBasedEstimate*/ 5000000,
			  /*inAlr*/ false);

			++feedbackCount;
		}

		// A single bit is added so that the state can become increasing at all.
		REQUIRE(lossBasedController.GetResult().bitrate == bitrateAfterLoss + 1);
	}

	SECTION("the delay based estimate caps this one while the network is fine")
	{
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.observationWindowSize = 15, .minNumObservations = 1
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(600000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		// With no bound of the delay based path, nothing holds the estimate back.
		REQUIRE(lossBasedController.GetResult().bitrate > 600000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(2 * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
		  /*delayBasedEstimate*/ 500000,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate == 500000);
	}

	SECTION("a strong signal of overuse backs the estimate off to what the link delivers")
	{
		constexpr int64_t AcknowledgedBitrate{ 300000 };

		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.observationWindowSize = 15, .minNumObservations = 1
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(600000);
		lossBasedController.SetAcknowledgedBitrate(AcknowledgedBitrate);

		// Half of the packets lost, and then every one of them.
		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 1),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);
		lossBasedController.UpdateBitrateEstimate(
		  createLostResults(2 * ObservationDurationLowerBoundUs, /*numPackets*/ 2),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate <= AcknowledgedBitrate);
	}

	SECTION("the bound the observed loss puts on the estimate makes it decreasing")
	{
		// A single candidate that changes nothing, so that the only thing that can
		// move the estimate is that bound, brought within reach by the balance.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.candidateFactors                  = { 1.0 },
			.observationWindowSize             = 2,
			.minNumObservations                = 1,
			.immediateUpperBoundBitrateBalance = 10000
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateLimits(/*minBitrate*/ 10000, /*maxBitrate*/ 1000000000);
		lossBasedController.SetBitrateEstimate(500000);
		lossBasedController.SetAcknowledgedBitrate(100000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 10, /*numLostPackets*/ 1),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		// 10000 / (0.1 - 0.05).
		REQUIRE(lossBasedController.GetResult().bitrate == 200000);
		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);
	}

	SECTION("the bound that what the link delivers puts on the estimate makes it increasing")
	{
		constexpr int64_t AcknowledgedBitrate{ 200000 };

		// A single candidate that changes nothing, and a lower bound well above
		// what the link delivers, so that only that bound can move the estimate.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.candidateFactors            = { 1.0 },
			.observationWindowSize       = 2,
			.minNumObservations          = 1,
			.lowerBoundByAckedRateFactor = 10.0
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateLimits(/*minBitrate*/ 10000, /*maxBitrate*/ 1000000000);
		lossBasedController.SetBitrateEstimate(500000);
		lossBasedController.SetAcknowledgedBitrate(1000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 1),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);

		// The link still loses as much, but delivers a lot more.
		lossBasedController.SetAcknowledgedBitrate(AcknowledgedBitrate);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(2 * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 1),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ false);

		REQUIRE(lossBasedController.GetResult().bitrate == AcknowledgedBitrate * 10);
		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::INCREASING);
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

	SECTION("while not filling the link the estimate grows slowly from the bound loss put on it")
	{
		// Coming down to that bound in one step rather than gradually leaves the
		// estimate where the link already is, so what follows is a slow rampup and
		// not the jump that a deeper backoff would allow.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.appendUpperBoundCandidateInAlr = true, .observationWindowSize = 2, .minNumObservations = 1
		};

		RTC::BWE::LossBasedController lossBasedController(options);

		lossBasedController.SetBitrateEstimate(1000000);
		lossBasedController.SetAcknowledgedBitrate(150000);

		lossBasedController.UpdateBitrateEstimate(
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 1),
		  RTC::BWE::Types::BitrateInfinite,
		  /*inAlr*/ true);

		REQUIRE(lossBasedController.GetResult().state == RTC::BWE::LossBasedController::State::DECREASING);

		const int64_t bitrateAfterLoss = lossBasedController.GetResult().bitrate;

		for (int64_t i{ 1 }; i <= 3; ++i)
		{
			lossBasedController.UpdateBitrateEstimate(
			  createResults(i * ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0),
			  RTC::BWE::Types::BitrateInfinite,
			  /*inAlr*/ true);
		}

		REQUIRE(lossBasedController.GetResult().bitrate < 2 * bitrateAfterLoss);
	}

	SECTION("what the link delivers stops being a candidate when told so")
	{
		// With the acknowledged bitrate not bounding the estimate from below, the
		// only way it can reach the estimate is by being a candidate of its own.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions withItOptions{
			.observationWindowSize = 2, .minNumObservations = 1, .lowerBoundByAckedRateFactor = 0.0
		};
		const RTC::BWE::LossBasedController::LossBasedControllerOptions withoutItOptions{
			.appendAcknowledgedRateCandidate = false,
			.observationWindowSize           = 2,
			.minNumObservations              = 1,
			.lowerBoundByAckedRateFactor     = 0.0
		};

		RTC::BWE::LossBasedController withIt(withItOptions);
		RTC::BWE::LossBasedController withoutIt(withoutItOptions);

		// The very same feedback goes to both, so the option is the only difference.
		const auto packetResults =
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0);

		withIt.SetBitrateEstimate(600000);
		withIt.SetAcknowledgedBitrate(1000000);
		withIt.UpdateBitrateEstimate(packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		withoutIt.SetBitrateEstimate(600000);
		withoutIt.SetAcknowledgedBitrate(1000000);
		withoutIt.UpdateBitrateEstimate(packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		// Without that candidate the estimate can only walk up by the factors:
		// 600000 * 1.02.
		REQUIRE(withoutIt.GetResult().bitrate == 612000);
		REQUIRE(withIt.GetResult().bitrate > withoutIt.GetResult().bitrate);
	}

	SECTION("what the link delivers is a candidate while not filling it when told so")
	{
		const RTC::BWE::LossBasedController::LossBasedControllerOptions withoutItOptions{
			.observationWindowSize = 2, .minNumObservations = 1, .lowerBoundByAckedRateFactor = 0.0
		};
		const RTC::BWE::LossBasedController::LossBasedControllerOptions withItOptions{
			.observationWindowSize       = 2,
			.minNumObservations          = 1,
			.lowerBoundByAckedRateFactor = 0.0,
			.notUseAckedRateInAlr        = false
		};

		RTC::BWE::LossBasedController withoutIt(withoutItOptions);
		RTC::BWE::LossBasedController withIt(withItOptions);

		const auto packetResults =
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0);

		withoutIt.SetBitrateEstimate(600000);
		withoutIt.SetAcknowledgedBitrate(1000000);
		withoutIt.UpdateBitrateEstimate(packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ true);

		withIt.SetBitrateEstimate(600000);
		withIt.SetAcknowledgedBitrate(1000000);
		withIt.UpdateBitrateEstimate(packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ true);

		REQUIRE(withoutIt.GetResult().bitrate == 612000);
		REQUIRE(withIt.GetResult().bitrate > withoutIt.GetResult().bitrate);
	}

	SECTION("the delay based estimate stops being a candidate when told so")
	{
		constexpr int64_t DelayBasedEstimate{ 1000000 };

		const RTC::BWE::LossBasedController::LossBasedControllerOptions withoutItOptions{
			.appendDelayBasedEstimateCandidate = false, .observationWindowSize = 2, .minNumObservations = 1
		};

		RTC::BWE::LossBasedController withIt(shortObservationOptions);
		RTC::BWE::LossBasedController withoutIt(withoutItOptions);

		const auto packetResults =
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0);

		withIt.SetBitrateEstimate(600000);
		withIt.UpdateBitrateEstimate(packetResults, DelayBasedEstimate, /*inAlr*/ false);

		withoutIt.SetBitrateEstimate(600000);
		withoutIt.UpdateBitrateEstimate(packetResults, DelayBasedEstimate, /*inAlr*/ false);

		// The delay based estimate still bounds the estimate from above either way,
		// so the option only decides whether it can be reached in a single step.
		REQUIRE(withoutIt.GetResult().bitrate == 612000);
		REQUIRE(withIt.GetResult().bitrate == DelayBasedEstimate);
	}

	SECTION("measuring loss by packets ignores how much each lost packet carried")
	{
		const RTC::BWE::LossBasedController::LossBasedControllerOptions byPacketsOptions{
			.observationWindowSize = 2, .minNumObservations = 1, .useByteLossRate = false
		};

		RTC::BWE::LossBasedController byBytes(shortObservationOptions);
		RTC::BWE::LossBasedController byPackets(byPacketsOptions);

		// Two packets whose sizes are wildly apart with the tiny one lost: by
		// bytes next to nothing was lost, by packets half of everything was.
		std::vector<RTC::BWE::Types::PacketResult> packetResults(2);

		packetResults[0].sentPacket.sequenceNumber = sequenceNumber++;
		packetResults[0].sentPacket.size           = PacketSizeBytes;
		packetResults[0].sentPacket.sendTimeUs     = 0;
		packetResults[0].receiveTimeUs             = ObservationDurationLowerBoundUs;

		packetResults[1].sentPacket.sequenceNumber = sequenceNumber++;
		packetResults[1].sentPacket.size           = 50;
		packetResults[1].sentPacket.sendTimeUs     = ObservationDurationLowerBoundUs;

		byBytes.SetBitrateEstimate(600000);
		byBytes.UpdateBitrateEstimate(packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		byPackets.SetBitrateEstimate(600000);
		byPackets.UpdateBitrateEstimate(packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		REQUIRE(byBytes.GetResult().bitrate > byPackets.GetResult().bitrate);
	}

	SECTION("an estimate brought down to its bounds stops describing the link")
	{
		constexpr int64_t DelayBasedEstimate{ 500000 };

		const RTC::BWE::LossBasedController::LossBasedControllerOptions keepingItOptions{
			.observationWindowSize = 2, .minNumObservations = 1, .boundBestCandidate = false
		};

		RTC::BWE::LossBasedController dropping(shortObservationOptions);
		RTC::BWE::LossBasedController keeping(keepingItOptions);

		const auto boundedResults =
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0);
		const auto freeResults =
		  createResults(ObservationDurationLowerBoundUs, /*numPackets*/ 2, /*numLostPackets*/ 0);

		dropping.SetBitrateEstimate(600000);
		keeping.SetBitrateEstimate(600000);

		// The delay based estimate brings the chosen candidate down, which both
		// report as their estimate.
		dropping.UpdateBitrateEstimate(boundedResults, DelayBasedEstimate, /*inAlr*/ false);
		keeping.UpdateBitrateEstimate(boundedResults, DelayBasedEstimate, /*inAlr*/ false);

		REQUIRE(dropping.GetResult().bitrate == DelayBasedEstimate);
		REQUIRE(keeping.GetResult().bitrate == DelayBasedEstimate);

		// Once nothing bounds them anymore, the one that kept the candidate as its
		// description of the link carries on from there instead of from the bound.
		dropping.UpdateBitrateEstimate(freeResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);
		keeping.UpdateBitrateEstimate(freeResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		REQUIRE(dropping.GetResult().bitrate == 510000);
		REQUIRE(keeping.GetResult().bitrate == 624240);
	}

	SECTION("the longer since the estimate was last reduced the more it may grow")
	{
		constexpr int64_t DelayBasedEstimate{ 5000000 };

		// A single candidate above what the window that follows a decrease allows,
		// so that the only thing deciding where the estimate lands is that bound.
		// The other two candidates are left out for the same reason.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions plainOptions{
			.candidateFactors                  = { 1.5 },
			.appendAcknowledgedRateCandidate   = false,
			.appendDelayBasedEstimateCandidate = false,
			.observationWindowSize             = 2,
			.minNumObservations                = 1,
			.lowerBoundByAckedRateFactor       = 0.0
		};
		const RTC::BWE::LossBasedController::LossBasedControllerOptions acceleratedOptions{
			.candidateFactors                  = { 1.5 },
			.rampupAccelerationMaxFactor       = 0.2,
			.appendAcknowledgedRateCandidate   = false,
			.appendDelayBasedEstimateCandidate = false,
			.observationWindowSize             = 2,
			.minNumObservations                = 1,
			.lowerBoundByAckedRateFactor       = 0.0
		};

		RTC::BWE::LossBasedController plain(plainOptions);
		RTC::BWE::LossBasedController accelerated(acceleratedOptions);

		const auto firstResults =
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0);
		// Late enough for both the hold and the window that follows a decrease to
		// be over, which is what leaves that bound as the only one in the way.
		const auto lateResults =
		  createResults(/*firstSendTimeUs*/ 400 * 1000, /*numPackets*/ 2, /*numLostPackets*/ 0);

		plain.SetBitrateEstimate(600000);
		accelerated.SetBitrateEstimate(600000);

		plain.UpdateBitrateEstimate(firstResults, DelayBasedEstimate, /*inAlr*/ false);
		accelerated.UpdateBitrateEstimate(firstResults, DelayBasedEstimate, /*inAlr*/ false);

		// High enough that what the link is known to deliver isn't what holds the
		// estimate back.
		plain.SetAcknowledgedBitrate(5000000);
		accelerated.SetAcknowledgedBitrate(5000000);

		plain.UpdateBitrateEstimate(lateResults, DelayBasedEstimate, /*inAlr*/ false);
		accelerated.UpdateBitrateEstimate(lateResults, DelayBasedEstimate, /*inAlr*/ false);

		// 900000 * 1.3, which is what that window allows on its own.
		REQUIRE(plain.GetResult().bitrate == 1170000);
		REQUIRE(accelerated.GetResult().bitrate > plain.GetResult().bitrate);
	}

	SECTION("the estimate grows despite the observed loss when the guard is off")
	{
		// The counterpart of the case above, with the same single candidate above
		// the current estimate and the guard that holds it back disabled.
		const RTC::BWE::LossBasedController::LossBasedControllerOptions options{
			.candidateFactors                             = { 1.2 },
			.observationWindowSize                        = 2,
			.minNumObservations                           = 1,
			.notIncreaseIfInherentLossLessThanAverageLoss = false
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

		REQUIRE(lossBasedController.GetResult().bitrate == 720000);
	}

	SECTION("this controller can be kept from taking over during the start phase")
	{
		const RTC::BWE::LossBasedController::LossBasedControllerOptions notInStartPhaseOptions{
			.observationWindowSize = 2, .minNumObservations = 1, .useInStartPhase = false
		};

		RTC::BWE::LossBasedController inStartPhase(shortObservationOptions);
		RTC::BWE::LossBasedController notInStartPhase(notInStartPhaseOptions);

		const auto packetResults =
		  createResults(/*firstSendTimeUs*/ 0, /*numPackets*/ 2, /*numLostPackets*/ 0);

		inStartPhase.SetBitrateEstimate(600000);
		inStartPhase.UpdateBitrateEstimate(
		  packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		notInStartPhase.SetBitrateEstimate(600000);
		notInStartPhase.UpdateBitrateEstimate(
		  packetResults, RTC::BWE::Types::BitrateInfinite, /*inAlr*/ false);

		// Both have observed enough to say something, but only one of them is
		// allowed to say it that early.
		REQUIRE(inStartPhase.IsReady());
		REQUIRE(inStartPhase.IsReadyToUseInStartPhase());
		REQUIRE(notInStartPhase.IsReady());
		REQUIRE(!notInStartPhase.IsReadyToUseInStartPhase());
	}
}
