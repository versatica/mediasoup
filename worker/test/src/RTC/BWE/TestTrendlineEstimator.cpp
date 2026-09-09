#include "common.hpp"
#include "RTC/BWE/TrendlineEstimator.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE TrendlineEstimator", "[bwe][trendlineestimator]")
{
	// Number of packets a test is given.
	constexpr size_t PacketCount{ 25 };
	// Time between two consecutive groups of packets.
	constexpr int64_t SendDeltaUs{ 20 * 1000 };
	constexpr int64_t InitialArrivalTimeUs{ 987654321000 };

	// Feed groups delivered at `deliveryPace` times the pace they were sent at,
	// until the state changes or the packets are exhausted.
	//
	// Both `count` and `arrivalTimeUs` are shared by every run within a test, so
	// that consecutive runs consume a single budget of packets and carry on from
	// the network conditions left by the previous one.
	auto runUntilStateChange = [](
	                             RTC::BWE::TrendlineEstimator& trendlineEstimator,
	                             double deliveryPace,
	                             size_t& count,
	                             size_t packetCount,
	                             int64_t& arrivalTimeUs) -> void
	{
		const auto initialState   = trendlineEstimator.GetState();
		const auto arrivalDeltaUs = static_cast<int64_t>(SendDeltaUs * deliveryPace);

		for (; count < packetCount; ++count)
		{
			arrivalTimeUs += arrivalDeltaUs;

			trendlineEstimator.Update(SendDeltaUs, arrivalDeltaUs, arrivalTimeUs);

			if (trendlineEstimator.GetState() != initialState)
			{
				return;
			}
		}
	};

	SECTION("a network delivering at the pace it's fed stays in normal state")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		size_t count{ 1 };
		int64_t arrivalTimeUs{ InitialArrivalTimeUs };

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);

		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.0, count, PacketCount, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
		// Every packet was processed, so the state never changed.
		REQUIRE(count == PacketCount);
	}

	SECTION("a 10% slower delivery is detected as overusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		size_t count{ 1 };
		int64_t arrivalTimeUs{ InitialArrivalTimeUs };

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);

		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.1, count, PacketCount, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);

		// The state is kept while the delay keeps growing.
		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.1, count, PacketCount, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);
		REQUIRE(count == PacketCount);
	}

	SECTION("a 15% faster delivery is detected as underusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		size_t count{ 1 };
		int64_t arrivalTimeUs{ InitialArrivalTimeUs };

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);

		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 0.85, count, PacketCount, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::UNDERUSING);

		// The state is kept while the delay keeps shrinking.
		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 0.85, count, PacketCount, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::UNDERUSING);
		REQUIRE(count == PacketCount);
	}

	SECTION("an isolated delay spike is not detected as overusing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		size_t count{ 1 };
		int64_t arrivalTimeUs{ InitialArrivalTimeUs };

		// Fill the regression window with a well behaved network.
		runUntilStateChange(trendlineEstimator, /*deliveryPace*/ 1.0, count, PacketCount, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);

		// A single group arrives 50 ms late and the very next one recovers that same
		// time, which is a jitter spike and not a queue that keeps growing.
		arrivalTimeUs += SendDeltaUs + (50 * 1000);
		trendlineEstimator.Update(SendDeltaUs, SendDeltaUs + (50 * 1000), arrivalTimeUs);

		arrivalTimeUs += SendDeltaUs - (50 * 1000);
		trendlineEstimator.Update(SendDeltaUs, SendDeltaUs - (50 * 1000), arrivalTimeUs);

		// This is the hysteresis being tested: overuse requires the condition to
		// persist over more than one sample.
		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}

	SECTION("recovers to normal once the delay stops growing")
	{
		RTC::BWE::TrendlineEstimator trendlineEstimator;
		size_t count{ 1 };
		int64_t arrivalTimeUs{ InitialArrivalTimeUs };

		runUntilStateChange(
		  trendlineEstimator, /*deliveryPace*/ 1.1, count, /*packetCount*/ 100, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::OVERUSING);

		// The delay stops growing but doesn't shrink either, so the accumulated
		// delay flattens out and the slope goes back to zero.
		runUntilStateChange(
		  trendlineEstimator, /*deliveryPace*/ 1.0, count, /*packetCount*/ 100, arrivalTimeUs);

		REQUIRE(trendlineEstimator.GetState() == RTC::BWE::Types::BandwidthUsage::NORMAL);
	}
}
