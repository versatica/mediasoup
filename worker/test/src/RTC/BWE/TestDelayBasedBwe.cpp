#include "common.hpp"
#include "RTC/BWE/BweTypes.hpp"
#include "RTC/BWE/DelayBasedBwe.hpp"
#include "RTC/BWE/RobustThroughputEstimator.hpp"
#include "test/include/RTC/BWE/helpers/LinkSimulator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

SCENARIO("BWE DelayBasedBwe", "[bwe][delaybasedbwe]")
{
	// The clock starts well away from zero so that a mistake taking a time for a
	// duration doesn't go unnoticed.
	constexpr int64_t InitialTimeUs{ 100000000 };

	// Holds everything a test needs: the link being simulated, the estimators
	// under test, and the latest bitrate they reported.
	struct SimulatedTransport
	{
		/**
		 * Feed a single packet, taking it through both estimators.
		 */
		void IncomingFeedback(int64_t arrivalTimeUs, int64_t sendTimeUs, size_t payloadSize)
		{
			RTC::BWE::Types::PacketResult packetResult;

			packetResult.receiveTimeUs             = arrivalTimeUs + this->arrivalTimeOffsetUs;
			packetResult.sentPacket.sendTimeUs     = sendTimeUs;
			packetResult.sentPacket.size           = payloadSize;
			packetResult.sentPacket.sequenceNumber = this->nextSequenceNumber++;

			RTC::BWE::Types::TransportPacketsFeedback feedback;

			feedback.feedbackTimeUs = this->nowUs;
			feedback.packetFeedbacks.push_back(packetResult);

			this->throughputEstimator.IncomingPacketFeedbackVector(feedback.SortedByReceiveTime());

			const auto result = this->delayBasedBwe.IncomingPacketFeedbackVector(
			  feedback,
			  this->throughputEstimator.GetBitrate(),
			  /*probeBitrate*/ std::nullopt,
			  /*networkEstimate*/ std::nullopt,
			  /*inAlr*/ false);

			if (result.updated)
			{
				this->latestBitrate = result.targetBitrate;
				this->updated       = true;
			}
		}

		/**
		 * Generate a frame at the given bitrate, push it through the link and give
		 * it to the estimators.
		 *
		 * @returns Whether the estimate dropped below what was being sent, which is
		 *   how an overuse shows up.
		 */
		bool GenerateAndProcessFrame(int64_t bitrateBps)
		{
			this->linkSimulator.SetBitrateBps(bitrateBps);

			std::vector<RTC::BWE::Types::PacketResult> packetResults;

			const int64_t nextTimeUs =
			  this->linkSimulator.GenerateFrame(this->nowUs, this->nextSequenceNumber, packetResults);

			if (packetResults.empty())
			{
				return false;
			}

			bool overuse{ false };

			this->updated = false;

			// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
			this->nowUs = packetResults.back().receiveTimeUs.value();

			for (auto& packetResult : packetResults)
			{
				// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
				packetResult.receiveTimeUs = packetResult.receiveTimeUs.value() + this->arrivalTimeOffsetUs;
			}

			this->throughputEstimator.IncomingPacketFeedbackVector(packetResults);

			RTC::BWE::Types::TransportPacketsFeedback feedback;

			feedback.packetFeedbacks = packetResults;
			feedback.feedbackTimeUs  = this->nowUs;

			const auto result = this->delayBasedBwe.IncomingPacketFeedbackVector(
			  feedback,
			  this->throughputEstimator.GetBitrate(),
			  /*probeBitrate*/ std::nullopt,
			  /*networkEstimate*/ std::nullopt,
			  /*inAlr*/ false);

			if (result.updated)
			{
				this->latestBitrate = result.targetBitrate;
				this->updated       = true;

				if (!this->firstUpdate && result.targetBitrate < bitrateBps)
				{
					overuse = true;
				}

				this->firstUpdate = false;
			}

			this->nowUs = nextTimeUs;

			return overuse;
		}

		/**
		 * Feed frames until the estimate settles above `targetBitrate` or the given
		 * number of frames is exhausted.
		 *
		 * @returns The bitrate it settled at.
		 */
		int64_t SteadyStateRun(
		  int64_t maxNumberOfFrames,
		  int64_t startBitrate,
		  int64_t minBitrate,
		  int64_t maxBitrate,
		  int64_t targetBitrate)
		{
			int64_t bitrateBps{ startBitrate };
			bool bitrateUpdateSeen{ false };

			for (int64_t i{ 0 }; i < maxNumberOfFrames; ++i)
			{
				const bool overuse = GenerateAndProcessFrame(bitrateBps);

				if (overuse)
				{
					REQUIRE(this->latestBitrate < maxBitrate);
					REQUIRE(this->latestBitrate > minBitrate);

					bitrateBps        = this->latestBitrate;
					bitrateUpdateSeen = true;
				}
				else if (this->updated)
				{
					bitrateBps    = this->latestBitrate;
					this->updated = false;
				}

				if (bitrateUpdateSeen && bitrateBps > targetBitrate)
				{
					break;
				}
			}

			REQUIRE(bitrateUpdateSeen);

			return bitrateBps;
		}

		void AddDefaultStream()
		{
			this->linkSimulator.AddStream(std::make_unique<bweHelpers::RtpStream>(30, 300000));
		}

		int64_t nowUs{ InitialTimeUs };
		bweHelpers::LinkSimulator linkSimulator{ 1000000, InitialTimeUs };
		RTC::BWE::RobustThroughputEstimator throughputEstimator;
		RTC::BWE::DelayBasedBwe delayBasedBwe;
		int64_t nextSequenceNumber{ 0 };
		int64_t arrivalTimeOffsetUs{ 0 };
		int64_t latestBitrate{ 0 };
		bool updated{ false };
		bool firstUpdate{ true };
	};

	// Runs the capacity drop scenario: converge at the initial capacity, halve
	// it, and measure how long the estimate takes to follow.
	auto capacityDropTestHelper = [](
	                                SimulatedTransport& simulatedTransport,
	                                int64_t numberOfStreams,
	                                int64_t expectedBitrateDropDeltaUs,
	                                int64_t receiverClockOffsetChangeUs)
	{
		constexpr int Framerate{ 30 };
		constexpr int64_t StartBitrate{ 900000 };
		constexpr int64_t MinExpectedBitrate{ 800000 };
		constexpr int64_t MaxExpectedBitrate{ 1100000 };
		constexpr int64_t InitialCapacityBps{ 1000000 };
		constexpr int64_t ReducedCapacityBps{ 500000 };

		int64_t steadyStateTime{ 0 };

		if (numberOfStreams <= 1)
		{
			steadyStateTime = 10;

			simulatedTransport.AddDefaultStream();
		}
		else
		{
			steadyStateTime = 10 * numberOfStreams;

			int64_t bitrateSum{ 0 };
			const int64_t bitrateDenom = numberOfStreams * (numberOfStreams - 1);

			for (int64_t i{ 0 }; i < numberOfStreams; ++i)
			{
				// The first stream gets half of the bitrate and the rest share the
				// other half.
				int64_t bitrate = StartBitrate / 2;

				if (i > 0)
				{
					bitrate = ((StartBitrate * i) + (bitrateDenom / 2)) / bitrateDenom;
				}

				simulatedTransport.linkSimulator.AddStream(
				  std::make_unique<bweHelpers::RtpStream>(Framerate, bitrate));

				bitrateSum += bitrate;
			}

			REQUIRE(bitrateSum == StartBitrate);
		}

		// Run in steady state until the estimate converges.
		simulatedTransport.linkSimulator.SetCapacityBps(InitialCapacityBps);

		int64_t bitrateBps = simulatedTransport.SteadyStateRun(
		  steadyStateTime * Framerate,
		  StartBitrate,
		  MinExpectedBitrate,
		  MaxExpectedBitrate,
		  InitialCapacityBps);

		REQUIRE(std::abs(bitrateBps - InitialCapacityBps) < 180000);

		simulatedTransport.updated = false;

		// An offset of the remote clock must not throw the estimator off.
		simulatedTransport.arrivalTimeOffsetUs += receiverClockOffsetChangeUs;

		// Halve the capacity and measure how long the estimate takes to follow.
		simulatedTransport.linkSimulator.SetCapacityBps(ReducedCapacityBps);

		const int64_t overuseStartTimeUs = simulatedTransport.nowUs;
		int64_t bitrateDropTimeUs{ -1 };

		for (int64_t i{ 0 }; i < 100 * numberOfStreams; ++i)
		{
			simulatedTransport.GenerateAndProcessFrame(bitrateBps);

			if (bitrateDropTimeUs == -1 && simulatedTransport.latestBitrate <= ReducedCapacityBps)
			{
				bitrateDropTimeUs = simulatedTransport.nowUs;
			}

			if (simulatedTransport.updated)
			{
				bitrateBps = simulatedTransport.latestBitrate;
			}
		}

		// NOTE: The elapsed time is truncated to whole milliseconds, which is the
		// resolution the tolerance below is calibrated for. Truncating each of the
		// two instants instead would make the result depend on where the clock
		// happens to sit within a millisecond, and this scenario lands right at the
		// edge of the tolerance.
		const int64_t bitrateDropDeltaMs = (bitrateDropTimeUs - overuseStartTimeUs) / 1000;

		REQUIRE(std::abs(bitrateDropDeltaMs - (expectedBitrateDropDeltaUs / 1000)) <= 33);
	};

	SECTION("a fresh result reports a normal network")
	{
		const RTC::BWE::DelayBasedBwe::Result result;

		REQUIRE(result.delayDetectorState == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}

	SECTION("the bitrate climbs to 500 kbps in the expected number of frames")
	{
		SimulatedTransport simulatedTransport;

		// The threshold corresponds to increasing roughly as
		// bitrate(i) = 1.04 * bitrate(i-1) + 1000 until it passes 500 kbps,
		// starting from about 30 kbps.
		int64_t bitrateBps{ 30000 };
		int iterations{ 0 };

		simulatedTransport.AddDefaultStream();

		while (bitrateBps < 500000)
		{
			const bool overuse = simulatedTransport.GenerateAndProcessFrame(bitrateBps);

			if (overuse)
			{
				REQUIRE(simulatedTransport.latestBitrate > bitrateBps);

				bitrateBps                 = simulatedTransport.latestBitrate;
				simulatedTransport.updated = false;
			}
			else if (simulatedTransport.updated)
			{
				bitrateBps                 = simulatedTransport.latestBitrate;
				simulatedTransport.updated = false;
			}

			++iterations;
		}

		REQUIRE(iterations == 617);
	}

	SECTION("the estimate follows a capacity drop with one stream")
	{
		SimulatedTransport simulatedTransport;

		capacityDropTestHelper(
		  simulatedTransport,
		  /*numberOfStreams*/ 1,
		  /*expectedDropDeltaUs*/ 500 * 1000,
		  /*clockOffsetUs*/ 0);
	}

	SECTION("the estimate follows a capacity drop when the remote clock jumps forward")
	{
		SimulatedTransport simulatedTransport;

		capacityDropTestHelper(
		  simulatedTransport,
		  /*numberOfStreams*/ 1,
		  /*expectedDropDeltaUs*/ 867 * 1000,
		  /*clockOffsetUs*/ 30000 * 1000);
	}

	SECTION("the estimate follows a capacity drop when the remote clock jumps backwards")
	{
		SimulatedTransport simulatedTransport;

		capacityDropTestHelper(
		  simulatedTransport,
		  /*numberOfStreams*/ 1,
		  /*expectedDropDeltaUs*/ 933 * 1000,
		  /*clockOffsetUs*/ -30000 * 1000);
	}

	SECTION("the estimate follows a capacity drop within a wider margin")
	{
		SimulatedTransport simulatedTransport;

		// The very same scenario as the first capacity drop, expected against a
		// margin shifted by 33 ms. Upstream keeps both because the measured value
		// falls inside the two, and the parameter that used to tell them apart is
		// no longer read by anything.
		capacityDropTestHelper(
		  simulatedTransport,
		  /*numberOfStreams*/ 1,
		  /*expectedDropDeltaUs*/ 533 * 1000,
		  /*clockOffsetUs*/ 0);
	}

	SECTION("packets sent very close together are treated as one group")
	{
		constexpr int Framerate{ 50 };
		constexpr int64_t FrameIntervalUs{ 1000000 / Framerate };

		SimulatedTransport simulatedTransport;

		int64_t sendTimeUs{ 0 };

		// Six seconds of frames, enough for the first estimate to be produced.
		for (int i{ 0 }; i <= 6 * Framerate; ++i)
		{
			simulatedTransport.IncomingFeedback(simulatedTransport.nowUs, sendTimeUs, 1000);

			simulatedTransport.nowUs += FrameIntervalUs;
			sendTimeUs += FrameIntervalUs;
		}

		REQUIRE(simulatedTransport.updated);
		REQUIRE(simulatedTransport.latestBitrate >= 400000);

		// Batches of frames sent one tick apart, which the estimator must take as
		// a single group, while the time between batches grows to simulate overuse.
		constexpr int TimestampGroupLength{ 15 };

		for (int i{ 0 }; i < 100; ++i)
		{
			for (int j{ 0 }; j < TimestampGroupLength; ++j)
			{
				simulatedTransport.IncomingFeedback(simulatedTransport.nowUs, sendTimeUs, 100);

				simulatedTransport.nowUs += FrameIntervalUs / TimestampGroupLength;
				sendTimeUs += 1000;
			}

			simulatedTransport.nowUs += 10 * 1000;
			sendTimeUs += FrameIntervalUs - (TimestampGroupLength * 1000);
		}

		REQUIRE(simulatedTransport.updated);
		// The estimate must have been reduced.
		REQUIRE(simulatedTransport.latestBitrate < 400000);
	}

	SECTION("a short silence doesn't leave the estimate stale")
	{
		SimulatedTransport simulatedTransport;

		constexpr int Framerate{ 100 };
		constexpr int64_t FrameIntervalUs{ 1000000 / Framerate };
		// A client leaving and rejoining after 35 seconds.
		constexpr int64_t SilenceTimeUs{ 35 * 1000000 };

		int64_t sendTimeUs{ 0 };

		for (size_t i{ 0 }; i < 3000; ++i)
		{
			simulatedTransport.IncomingFeedback(simulatedTransport.nowUs, sendTimeUs, 1000);

			simulatedTransport.nowUs += FrameIntervalUs;
			sendTimeUs += FrameIntervalUs;
		}

		const auto bitrateBefore = simulatedTransport.delayBasedBwe.GetLatestEstimate();

		simulatedTransport.nowUs += SilenceTimeUs;
		sendTimeUs += SilenceTimeUs;

		for (size_t i{ 0 }; i < 24; ++i)
		{
			simulatedTransport.IncomingFeedback(simulatedTransport.nowUs, sendTimeUs, 1000);

			simulatedTransport.nowUs += 2 * FrameIntervalUs;
			sendTimeUs += FrameIntervalUs;
		}

		const auto bitrateAfter = simulatedTransport.delayBasedBwe.GetLatestEstimate();

		REQUIRE(bitrateBefore.has_value());
		REQUIRE(bitrateAfter.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(bitrateAfter.value() < bitrateBefore.value());
	}

	SECTION("a long silence doesn't leave the estimate stale")
	{
		SimulatedTransport simulatedTransport;

		constexpr int Framerate{ 100 };
		constexpr int64_t FrameIntervalUs{ 1000000 / Framerate };
		// A client rejoining a multiple of 64 seconds later, which is what used to
		// produce no difference at all in the send times.
		constexpr int64_t SilenceTimeUs{ 10 * 64 * 1000000 };

		int64_t sendTimeUs{ 0 };

		for (size_t i{ 0 }; i < 3000; ++i)
		{
			simulatedTransport.IncomingFeedback(simulatedTransport.nowUs, sendTimeUs, 1000);

			simulatedTransport.nowUs += FrameIntervalUs;
			sendTimeUs += FrameIntervalUs;
		}

		const auto bitrateBefore = simulatedTransport.delayBasedBwe.GetLatestEstimate();

		simulatedTransport.nowUs += SilenceTimeUs;
		sendTimeUs += SilenceTimeUs;

		for (size_t i{ 0 }; i < 24; ++i)
		{
			simulatedTransport.IncomingFeedback(simulatedTransport.nowUs, sendTimeUs, 1000);

			simulatedTransport.nowUs += 2 * FrameIntervalUs;
			sendTimeUs += FrameIntervalUs;
		}

		const auto bitrateAfter = simulatedTransport.delayBasedBwe.GetLatestEstimate();

		REQUIRE(bitrateBefore.has_value());
		REQUIRE(bitrateAfter.has_value());
		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		REQUIRE(bitrateAfter.value() < bitrateBefore.value());
	}

	SECTION("an overuse right at the start is detected")
	{
		constexpr int64_t StartBitrate{ 300000 };
		constexpr int64_t InitialCapacity{ 200000 };
		// A high frame rate so that many packets go out in a short time.
		constexpr int Fps{ 90 };

		SimulatedTransport simulatedTransport;

		simulatedTransport.linkSimulator.AddStream(
		  std::make_unique<bweHelpers::RtpStream>(Fps, StartBitrate));
		simulatedTransport.linkSimulator.SetCapacityBps(InitialCapacity);

		// Needed to initialize the rate control.
		simulatedTransport.delayBasedBwe.SetStartBitrate(StartBitrate);

		int64_t bitrateBps{ StartBitrate };
		bool seenOveruse{ false };

		// Forty frames, which is a third of a second at this frame rate.
		for (int i{ 0 }; i < 40; ++i)
		{
			const bool overuse = simulatedTransport.GenerateAndProcessFrame(bitrateBps);

			if (overuse)
			{
				REQUIRE(simulatedTransport.updated);
				REQUIRE(simulatedTransport.latestBitrate <= InitialCapacity);
				REQUIRE(simulatedTransport.latestBitrate > 0.8 * InitialCapacity);

				seenOveruse = true;

				break;
			}
			else if (simulatedTransport.updated)
			{
				bitrateBps                 = simulatedTransport.latestBitrate;
				simulatedTransport.updated = false;
			}
		}

		REQUIRE(seenOveruse);
		REQUIRE(simulatedTransport.latestBitrate <= InitialCapacity);
		REQUIRE(simulatedTransport.latestBitrate > 0.8 * InitialCapacity);
	}

	SECTION("sub-millisecond timestamps don't fake a delay build up")
	{
		SimulatedTransport simulatedTransport;

		// Send times are 0.000, 9.725, 20.000, 29.725... ms and arrival times are
		// 0.500, 10.000, 20.500, 30.000... ms, so the send deltas alternate between
		// 9.750 and 10.250 ms and the arrival ones between 9.500 and 10.500 ms. No
		// delay builds up, so this must never be read as overuse. Rounding the
		// deltas to whole milliseconds would make every send delta 10 ms while some
		// arrival ones would round up to 11 ms, faking exactly that.
		int64_t lastBitrate{ simulatedTransport.latestBitrate };

		for (int i{ 0 }; i < 1000; ++i)
		{
			simulatedTransport.nowUs += 500;
			simulatedTransport.IncomingFeedback(
			  simulatedTransport.nowUs, simulatedTransport.nowUs - 500, 1000);

			simulatedTransport.nowUs += 9500;
			simulatedTransport.IncomingFeedback(
			  simulatedTransport.nowUs, simulatedTransport.nowUs - 250, 1000);

			simulatedTransport.nowUs += 10000;

			// The bitrate must never decrease.
			REQUIRE(lastBitrate <= simulatedTransport.latestBitrate);

			lastBitrate = simulatedTransport.latestBitrate;
		}
	}
}
