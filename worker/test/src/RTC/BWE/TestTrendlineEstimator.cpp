#include "common.hpp"
#include "RTC/BWE/TrendlineEstimator.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE TrendlineEstimator", "[bwe][trendlineestimator]")
{
	// Number of samples of the regression window, so the number of updates needed
	// before any trend can be computed.
	constexpr int WindowSize{ 20 };
	// Time between two consecutive groups of packets.
	constexpr int64_t SendDeltaUs{ 20000 };
	constexpr uint64_t InitialArrivalTimeUs{ 1000000000 };

	// Feed `count` groups whose arrival delta exceeds the send delta by
	// `extraDelayUs`, which is what a growing queue looks like. A value of 0 means
	// a network that is not queueing at all.
	auto feed = [](
	              RTC::BWE::TrendlineEstimator& trendlineEstimator,
	              int count,
	              int64_t extraDelayUs,
	              uint64_t& arrivalTimeUs) -> void
	{
		for (int i{ 0 }; i < count; ++i)
		{
			const int64_t arrivalDeltaUs = SendDeltaUs + extraDelayUs;

			arrivalTimeUs += arrivalDeltaUs;

			trendlineEstimator.Update(SendDeltaUs, arrivalDeltaUs, arrivalTimeUs);
		}
	};

	SECTION("starts in normal state")
	{
		const RTC::BWE::TrendlineEstimator trendlineEstimator;

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}

	SECTION("a network that doesn't queue stays in normal state")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		// Arrival deltas match send deltas exactly, so the accumulated delay never
		// moves and the slope stays at zero.
		feed(trendlineEstimator, 10 * WindowSize, /*extraDelayUs*/ 0, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}

	SECTION("no trend is computed before the window is full")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		// Even with a strongly growing delay, fewer samples than the window size
		// cannot produce a slope.
		feed(trendlineEstimator, WindowSize - 1, /*extraDelayUs*/ 5000, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}

	SECTION("a growing delay is detected as overusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		// Each group arrives 5 ms later than its send delta would suggest, so the
		// accumulated delay grows without bound: queues are filling up.
		feed(trendlineEstimator, 5 * WindowSize, /*extraDelayUs*/ 5000, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);
	}

	SECTION("a shrinking delay is detected as underusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		// Build up some accumulated delay first.
		feed(trendlineEstimator, 5 * WindowSize, /*extraDelayUs*/ 5000, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);

		// Now groups arrive earlier than their send delta, so the queues drain.
		feed(trendlineEstimator, 5 * WindowSize, /*extraDelayUs*/ -5000, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::UNDERUSING);
	}

	SECTION("an isolated delay spike is not detected as overusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		// Fill the window with a well behaved network.
		feed(trendlineEstimator, 5 * WindowSize, /*extraDelayUs*/ 0, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);

		// A single group arrives 50 ms late and the very next one recovers that same
		// time, which is a jitter spike and not a queue that keeps growing.
		feed(trendlineEstimator, 1, /*extraDelayUs*/ 50000, arrivalTimeUs);
		feed(trendlineEstimator, 1, /*extraDelayUs*/ -50000, arrivalTimeUs);

		// This is the hysteresis being tested: overuse requires the condition to
		// persist over more than one sample.
		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}

	SECTION("recovers to normal once the delay stops growing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		feed(trendlineEstimator, 5 * WindowSize, /*extraDelayUs*/ 5000, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);

		// The delay stops growing but doesn't shrink either, so the accumulated
		// delay flattens out and the slope goes back to zero.
		feed(trendlineEstimator, 10 * WindowSize, /*extraDelayUs*/ 0, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}
}
