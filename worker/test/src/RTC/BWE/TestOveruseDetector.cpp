#include "common.hpp"
#include "RTC/BWE/InterArrival.hpp"
#include "RTC/BWE/OveruseDetector.hpp"
#include "RTC/BWE/OveruseEstimator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numbers>

SCENARIO("BWE OveruseDetector", "[bwe][overusedetector]")
{
	// Ticks of a 90 kHz clock, which is what the timestamps of this test are.
	constexpr double RtpTimestampToUs{ 1000.0 / 90.0 };
	constexpr size_t PacketSize{ 1200 };

	/**
	 * Pseudo random generator with a fixed seed, used to add jitter to the
	 * arrival times of the cases below.
	 *
	 * It's a xorshift of 64 bits scaled by a constant, plus a normal
	 * distribution built out of two of its outputs by the Box-Muller transform.
	 *
	 * @remarks
	 * - It's copied from `Random` in `rtc_base/random.{h,cc}` of libwebrtc, and
	 *   that is the whole point of it: the cases below assert after how many
	 *   frames congestion is detected, and those counts come from
	 *   `overuse_detector_unittest.cc`, where they were produced by this exact
	 *   generator seeded with this exact value. Any other sequence of jitter
	 *   yields other counts.
	 */
	class TestRandom
	{
	public:
		explicit TestRandom(uint64_t seed) : state(seed)
		{
		}

		/**
		 * Uniformly distributed within [low, high].
		 */
		int64_t Rand(int64_t low, int64_t high)
		{
			const auto t          = static_cast<uint32_t>(high - low);
			const auto x          = static_cast<uint32_t>(NextOutput());
			const uint64_t result = (static_cast<uint64_t>(x) * (static_cast<uint64_t>(t) + 1)) >> 32;

			return static_cast<int64_t>(result) + low;
		}

		double Gaussian(double mean, double standardDeviation)
		{
			const double u1 = static_cast<double>(NextOutput()) / static_cast<double>(0xFFFFFFFFFFFFFFFF);
			const double u2 = static_cast<double>(NextOutput()) / static_cast<double>(0xFFFFFFFFFFFFFFFF);

			return mean + (standardDeviation * std::sqrt(-2 * std::log(u1)) *
			               std::cos(2 * std::numbers::pi * u2));
		}

	private:
		uint64_t NextOutput()
		{
			this->state ^= this->state >> 12;
			this->state ^= this->state << 25;
			this->state ^= this->state >> 27;

			return this->state * 2685821657736338717ULL;
		}

	private:
		uint64_t state;
	};

	int64_t nowMs{ 0 };
	int64_t receiveTimeMs{ 0 };
	uint32_t rtpTimestamp{ 10 * 90 };

	RTC::BWE::OveruseDetector overuseDetector;
	RTC::BWE::OveruseEstimator overuseEstimator;
	RTC::BWE::InterArrival interArrival(5 * 90, RtpTimestampToUs);
	TestRandom random(123456789);

	// Feed one packet through the whole chain, which is what this test drives:
	// the grouping, the estimate and the detection.
	const auto updateDetector = [&interArrival, &overuseEstimator, &overuseDetector](
	                              uint32_t rtpTimestamp, int64_t receiveTimeMs, size_t packetSize) -> void
	{
		const auto deltas = interArrival.ComputeDeltas(
		  rtpTimestamp, receiveTimeMs * 1000, receiveTimeMs * 1000, packetSize);

		if (!deltas.has_value())
		{
			return;
		}

		// NOLINTNEXTLINE(bugprone-unchecked-optional-access)
		const auto& value        = deltas.value();
		const double sendDeltaMs = value.timestampDelta / 90.0;

		overuseEstimator.Update(
		  static_cast<double>(value.arrivalDeltaUs) / 1000.0,
		  sendDeltaMs,
		  value.sizeDelta,
		  overuseDetector.GetState());
		overuseDetector.Detect(
		  overuseEstimator.GetOffsetMs(),
		  sendDeltaMs,
		  overuseEstimator.GetNumOfDeltas(),
		  receiveTimeMs * 1000);
	};

	// Run a long steady stream and count how many separate times congestion was
	// declared, which for a stream that isn't congested must be none.
	const auto run100000Samples =
	  [&nowMs, &receiveTimeMs, &rtpTimestamp, &overuseDetector, &random, &updateDetector](
	    int64_t packetsPerFrame, size_t packetSize, int64_t meanMs, int64_t standardDeviationMs) -> int64_t
	{
		int64_t uniqueOveruse{ 0 };
		int64_t lastOveruse{ -1 };

		for (int64_t i{ 0 }; i < 100000; ++i)
		{
			for (int64_t j{ 0 }; j < packetsPerFrame; ++j)
			{
				updateDetector(rtpTimestamp, receiveTimeMs, packetSize);
			}

			rtpTimestamp += meanMs * 90;
			nowMs += meanMs;
			const double jitter = random.Gaussian(0, static_cast<double>(standardDeviationMs));
			// NOTE: Truncated towards zero rather than rounded with std::llround(),
			// which rounds away from it. Half of these samples are negative, so the
			// two disagree on them and the counts asserted below would no longer
			// hold.
			// NOLINTNEXTLINE(bugprone-incorrect-roundings)
			const auto jitterMs = static_cast<int64_t>(jitter + 0.5);

			receiveTimeMs = std::max<int64_t>(receiveTimeMs, nowMs + jitterMs);

			if (overuseDetector.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING)
			{
				if (lastOveruse + 1 != i)
				{
					uniqueOveruse++;
				}

				lastOveruse = i;
			}
		}

		return uniqueOveruse;
	};

	// Send faster than the link takes and answer after how many frames it was
	// noticed, or -1 if it never was.
	const auto runUntilOveruse =
	  [&nowMs, &receiveTimeMs, &rtpTimestamp, &overuseDetector, &random, &updateDetector](
	    int64_t packetsPerFrame,
	    size_t packetSize,
	    int64_t meanMs,
	    int64_t standardDeviationMs,
	    int64_t driftPerFrameMs) -> int64_t
	{
		for (int64_t i{ 0 }; i < 1000; ++i)
		{
			for (int64_t j{ 0 }; j < packetsPerFrame; ++j)
			{
				updateDetector(rtpTimestamp, receiveTimeMs, packetSize);
			}

			rtpTimestamp += meanMs * 90;
			nowMs += meanMs + driftPerFrameMs;
			const double jitter = random.Gaussian(0, static_cast<double>(standardDeviationMs));
			// NOTE: Truncated towards zero rather than rounded with std::llround(),
			// which rounds away from it. Half of these samples are negative, so the
			// two disagree on them and the counts asserted below would no longer
			// hold.
			// NOLINTNEXTLINE(bugprone-incorrect-roundings)
			const auto jitterMs = static_cast<int64_t>(jitter + 0.5);

			receiveTimeMs = std::max<int64_t>(receiveTimeMs, nowMs + jitterMs);

			if (overuseDetector.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING)
			{
				return i + 1;
			}
		}

		return -1;
	};

	// What every case below does: run a steady stream that must never be called
	// congested, then add a drift and check after how many frames it is.
	const auto expectNoOveruseThenOveruseAfter = [&run100000Samples, &runUntilOveruse](
	                                               int64_t packetsPerFrame,
	                                               int64_t frameDurationMs,
	                                               int64_t driftPerFrameMs,
	                                               int64_t sigmaMs,
	                                               int64_t expectedFramesUntilOveruse) -> void
	{
		REQUIRE(run100000Samples(packetsPerFrame, PacketSize, frameDurationMs, sigmaMs) == 0);
		REQUIRE(
		  runUntilOveruse(packetsPerFrame, PacketSize, frameDurationMs, sigmaMs, driftPerFrameMs) ==
		  expectedFramesUntilOveruse);
	};

	SECTION("a steady 30 fps stream with no variance is never called congested")
	{
		constexpr uint32_t FrameDurationMs{ 33 };

		uint32_t timestamp = 10 * 90;

		for (int64_t i{ 0 }; i < 1000; ++i)
		{
			updateDetector(timestamp, nowMs, PacketSize);

			nowMs += FrameDurationMs;
			timestamp += FrameDurationMs * 90;

			REQUIRE(overuseDetector.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
		}
	}

	SECTION("nor is one whose packets arrive a few milliseconds early and late")
	{
		constexpr uint32_t FrameDurationMs{ 10 };

		uint32_t timestamp = 10 * 90;

		for (int64_t i{ 0 }; i < 1000; ++i)
		{
			updateDetector(timestamp, nowMs, PacketSize);

			timestamp += FrameDurationMs * 90;

			if (i % 2 != 0)
			{
				nowMs += FrameDurationMs - 5;
			}
			else
			{
				nowMs += FrameDurationMs + 5;
			}

			REQUIRE(overuseDetector.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
		}
	}

	SECTION("nor one whose timestamps are the ones moving early and late")
	{
		constexpr uint32_t FrameDurationMs{ 10 };

		uint32_t timestamp = 10 * 90;

		for (int64_t i{ 0 }; i < 1000; ++i)
		{
			updateDetector(timestamp, nowMs, PacketSize);

			nowMs += FrameDurationMs;

			if (i % 2 != 0)
			{
				timestamp += (FrameDurationMs - 5) * 90;
			}
			else
			{
				timestamp += (FrameDurationMs + 5) * 90;
			}

			REQUIRE(overuseDetector.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
		}
	}

	SECTION("2000 kbps at 30 fps with no variance")
	{
		expectNoOveruseThenOveruseAfter(6, 33, 1, 0, 7);
	}

	SECTION("100 kbps at 10 fps with no variance")
	{
		expectNoOveruseThenOveruseAfter(1, 100, 1, 0, 7);
	}

	SECTION("a delay that builds up over a few frames is noticed on the last one")
	{
		constexpr uint32_t FrameDurationMs{ 33 };
		constexpr uint32_t DriftPerFrameMs{ 1 };

		uint32_t timestamp = FrameDurationMs * 90;
		int64_t offset{ 0 };

		// Run enough samples to reach a steady state.
		for (int64_t i{ 0 }; i < 1000; ++i)
		{
			for (int64_t j{ 0 }; j < 6; ++j)
			{
				updateDetector(timestamp, nowMs, PacketSize);
			}

			timestamp += FrameDurationMs * 90;

			if (i % 2 != 0)
			{
				offset = random.Rand(0, 1);
				nowMs += FrameDurationMs - offset;
			}
			else
			{
				nowMs += FrameDurationMs + offset;
			}

			REQUIRE(overuseDetector.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
		}

		// Now send faster than the link takes, building up 30 ms in total.
		for (int64_t j{ 0 }; j < 3; ++j)
		{
			for (int64_t k{ 0 }; k < 6; ++k)
			{
				updateDetector(timestamp, nowMs, PacketSize);
			}

			nowMs += FrameDurationMs + (DriftPerFrameMs * 6);
			timestamp += FrameDurationMs * 90;

			REQUIRE(overuseDetector.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
		}

		updateDetector(timestamp, nowMs, PacketSize);

		REQUIRE(overuseDetector.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);
	}

	SECTION("30 kbps at 3 fps with low variance")
	{
		expectNoOveruseThenOveruseAfter(1, 333, 1, 3, 20);
	}

	SECTION("30 kbps at 3 fps with low variance and a fast drift")
	{
		expectNoOveruseThenOveruseAfter(1, 333, 100, 3, 4);
	}

	SECTION("30 kbps at 3 fps with high variance")
	{
		expectNoOveruseThenOveruseAfter(1, 333, 1, 10, 44);
	}

	SECTION("30 kbps at 3 fps with high variance and a fast drift")
	{
		expectNoOveruseThenOveruseAfter(1, 333, 100, 10, 4);
	}

	SECTION("100 kbps at 5 fps with low variance")
	{
		expectNoOveruseThenOveruseAfter(2, 200, 1, 3, 20);
	}

	SECTION("100 kbps at 5 fps with high variance")
	{
		expectNoOveruseThenOveruseAfter(2, 200, 1, 10, 44);
	}

	SECTION("100 kbps at 10 fps with low variance")
	{
		expectNoOveruseThenOveruseAfter(1, 100, 1, 3, 20);
	}

	SECTION("100 kbps at 10 fps with high variance")
	{
		expectNoOveruseThenOveruseAfter(1, 100, 1, 10, 44);
	}

	SECTION("300 kbps at 30 fps with low variance")
	{
		expectNoOveruseThenOveruseAfter(1, 33, 1, 3, 19);
	}

	SECTION("300 kbps at 30 fps with low variance and a fast drift")
	{
		expectNoOveruseThenOveruseAfter(1, 33, 10, 3, 5);
	}

	SECTION("300 kbps at 30 fps with high variance")
	{
		expectNoOveruseThenOveruseAfter(1, 33, 1, 10, 44);
	}

	SECTION("300 kbps at 30 fps with high variance and a fast drift")
	{
		expectNoOveruseThenOveruseAfter(1, 33, 10, 10, 10);
	}

	SECTION("1000 kbps at 30 fps with low variance")
	{
		expectNoOveruseThenOveruseAfter(3, 33, 1, 3, 19);
	}

	SECTION("1000 kbps at 30 fps with low variance and a fast drift")
	{
		expectNoOveruseThenOveruseAfter(3, 33, 10, 3, 5);
	}

	SECTION("1000 kbps at 30 fps with high variance")
	{
		expectNoOveruseThenOveruseAfter(3, 33, 1, 10, 44);
	}

	SECTION("1000 kbps at 30 fps with high variance and a fast drift")
	{
		expectNoOveruseThenOveruseAfter(3, 33, 10, 10, 10);
	}

	SECTION("2000 kbps at 30 fps with low variance")
	{
		expectNoOveruseThenOveruseAfter(6, 33, 1, 3, 19);
	}

	SECTION("2000 kbps at 30 fps with low variance and a fast drift")
	{
		expectNoOveruseThenOveruseAfter(6, 33, 10, 3, 5);
	}

	SECTION("2000 kbps at 30 fps with high variance")
	{
		expectNoOveruseThenOveruseAfter(6, 33, 1, 10, 44);
	}

	SECTION("2000 kbps at 30 fps with high variance and a fast drift")
	{
		expectNoOveruseThenOveruseAfter(6, 33, 10, 10, 10);
	}

	SECTION("the threshold adapts to what it keeps being shown")
	{
		constexpr double Offset{ 0.21 };
		constexpr double TsDeltaMs{ 3000.0 };
		constexpr int64_t BatchLength{ 10 };

		int64_t nowUs{ 0 };
		int64_t numDeltas{ 60 };

		// Whether congestion was declared at any point of the batch, which is what
		// every step below looks at.
		const auto runBatch = [&overuseDetector, &nowUs, &numDeltas](
		                        int64_t batchLength, double offsetMs) -> bool
		{
			bool overuseDetected{ false };

			for (int64_t i{ 0 }; i < batchLength; ++i)
			{
				if (overuseDetector.Detect(offsetMs, TsDeltaMs, numDeltas, nowUs) == RTC::BWE::Types::BandwidthUsage::OVERUSING)
				{
					overuseDetected = true;
				}

				++numDeltas;
				nowUs += 5 * 1000;
			}

			return overuseDetected;
		};

		// A positive offset triggers congestion.
		REQUIRE(runBatch(BatchLength, Offset));

		// A higher one forces the threshold up, and triggers it too.
		REQUIRE(runBatch(BatchLength, 1.1 * Offset));

		// So the one that triggered it at first no longer does.
		REQUIRE(!runBatch(BatchLength, Offset));

		// A low offset shown for long enough brings the threshold back down.
		REQUIRE(!runBatch(15 * BatchLength, 0.7 * Offset));

		// And then the original offset triggers it again.
		REQUIRE(runBatch(BatchLength, Offset));
	}

	SECTION("the threshold doesn't adapt to an isolated spike")
	{
		constexpr double Offset{ 1.0 };
		constexpr double LargeOffset{ 20.0 };
		constexpr double TsDeltaMs{ 3000.0 };
		constexpr int64_t BatchLength{ 10 };
		constexpr int64_t ShortBatchLength{ 3 };

		int64_t nowUs{ 0 };
		int64_t numDeltas{ 60 };

		const auto runBatch = [&overuseDetector, &nowUs, &numDeltas](
		                        int64_t batchLength, double offsetMs) -> bool
		{
			bool overuseDetected{ false };

			for (int64_t i{ 0 }; i < batchLength; ++i)
			{
				if (overuseDetector.Detect(offsetMs, TsDeltaMs, numDeltas, nowUs) == RTC::BWE::Types::BandwidthUsage::OVERUSING)
				{
					overuseDetected = true;
				}

				++numDeltas;
				nowUs += 5 * 1000;
			}

			return overuseDetected;
		};

		// A positive offset triggers congestion.
		runBatch(BatchLength, Offset);

		// A far larger one triggers it too, without moving the threshold much.
		nowUs += 100 * 1000;

		REQUIRE(runBatch(ShortBatchLength, LargeOffset));

		// So the ordinary offset still triggers it.
		REQUIRE(runBatch(BatchLength, Offset));
	}
}
