#include "common.hpp"
#include "RTC/BWE/TrendlineEstimator.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE TrendlineEstimator", "[bwe][trendlineestimator]")
{
	// Number of samples of the regression window.
	constexpr size_t WindowSize{ 20 };
	// Samples fed before giving up waiting for a state change. Since the window
	// holds 20 samples, this leaves 5 of margin for the estimator to react.
	constexpr size_t MaxSamples{ 25 };
	// Time between two consecutive groups of packets.
	constexpr int64_t SendDeltaUs{ 20000 };
	constexpr uint64_t InitialArrivalTimeUs{ 1000000000 };

	// Feed groups delivered at `deliveryPace` times the pace they were sent at,
	// until the state changes or `MaxSamples` are consumed. Returns how many
	// samples were fed.
	auto runUntilStateChange = [](
	                             RTC::BWE::TrendlineEstimator& trendlineEstimator,
	                             double deliveryPace,
	                             uint64_t& arrivalTimeUs) -> size_t
	{
		const auto initialState = trendlineEstimator.GetState();

		const auto arrivalDeltaUs = static_cast<int64_t>(SendDeltaUs * deliveryPace);

		size_t samples{ 0 };

		while (samples < MaxSamples)
		{
			arrivalTimeUs += arrivalDeltaUs;
			++samples;

			trendlineEstimator.Update(SendDeltaUs, arrivalDeltaUs, arrivalTimeUs);

			if (trendlineEstimator.GetState() != initialState)
			{
				break;
			}
		}

		return samples;
	};

	SECTION("starts in normal state")
	{
		const RTC::BWE::TrendlineEstimator trendlineEstimator;

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}

	SECTION("a network delivering at the pace it's fed stays in normal state")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		const auto samples = runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.0, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
		// Every sample was consumed, so the state never changed.
		REQUIRE(samples == MaxSamples);
	}

	SECTION("a 10% slower delivery is detected as overusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		const auto samples = runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.1, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);
		// It cannot be detected before the regression window is full, and it must
		// be detected shortly after.
		REQUIRE(samples >= WindowSize);
		REQUIRE(samples < MaxSamples);
	}

	SECTION("a 15% faster delivery is detected as underusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		const auto samples =
		  runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 0.85, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::UNDERUSING);
		REQUIRE(samples >= WindowSize);
		REQUIRE(samples < MaxSamples);
	}

	SECTION("the overusing state is kept while the delay keeps growing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.1, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);

		const auto samples = runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.1, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);
		// No state change, so every sample was consumed.
		REQUIRE(samples == MaxSamples);
	}

	SECTION("the underusing state is kept while the delay keeps shrinking")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 0.85, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::UNDERUSING);

		const auto samples =
		  runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 0.85, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::UNDERUSING);
		// No state change, so every sample was consumed.
		REQUIRE(samples == MaxSamples);
	}

	SECTION("an isolated delay spike is not detected as overusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		// Fill the regression window with a well behaved network.
		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.0, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);

		// A single group arrives 50 ms late and the very next one recovers that same
		// time, which is a jitter spike and not a queue that keeps growing.
		arrivalTimeUs += SendDeltaUs + 50000;
		trendlineEstimator.Update(SendDeltaUs, SendDeltaUs + 50000, arrivalTimeUs);

		arrivalTimeUs += SendDeltaUs - 50000;
		trendlineEstimator.Update(SendDeltaUs, SendDeltaUs - 50000, arrivalTimeUs);

		// This is the hysteresis being tested: overuse requires the condition to
		// persist over more than one sample.
		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}

	SECTION("recovers to normal once the delay stops growing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		uint64_t arrivalTimeUs{ InitialArrivalTimeUs };

		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.1, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);

		// The delay stops growing but doesn't shrink either, so the accumulated
		// delay flattens out and the slope goes back to zero.
		const auto samples = runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.0, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
		REQUIRE(samples < MaxSamples);
	}
}
