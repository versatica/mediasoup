#include "common.hpp"
#include "RTC/BWE/AlrDetector.hpp"
#include "mocks/include/MockShared.hpp"
#include <catch2/catch_test_macros.hpp>
#include <memory>

SCENARIO("BWE AlrDetector", "[bwe][alrdetector]")
{
	constexpr int64_t InitialTimeUs{ 1000 * 1000 };
	constexpr int64_t EstimatedBitrate{ 300000 };
	// Span each step of the simulated traffic covers.
	constexpr int64_t StepUs{ 10 * 1000 };

	int64_t nowUs{ InitialTimeUs };

	mocks::MockShared shared(/*getTimeUs*/
	                         [&nowUs]() -> int64_t
	                         {
		                         return nowUs;
	                         });

	// Send traffic at the given percentage of the estimated bitrate for the given
	// span of time, in steps so that the detector sees a steady pace rather than
	// one huge send.
	const auto sendTraffic =
	  [&nowUs](RTC::BWE::AlrDetector& alrDetector, int64_t usagePercentage, int64_t durationUs) -> void
	{
		for (int64_t elapsedUs{ 0 }; elapsedUs < durationUs; elapsedUs += StepUs)
		{
			nowUs += StepUs;

			const int64_t bytesSent = (EstimatedBitrate * usagePercentage * StepUs) / (100 * 8 * 1000000);

			alrDetector.OnBytesSent(bytesSent, nowUs);
		}
	};

	SECTION("the link stops being filled and starts being filled again")
	{
		RTC::BWE::AlrDetector alrDetector(std::addressof(shared));

		alrDetector.SetEstimatedBitrate(EstimatedBitrate);

		REQUIRE(!alrDetector.GetAlrStartTimeUs().has_value());

		// Sending close to what the estimate allows is filling the link.
		sendTraffic(alrDetector, /*usagePercentage*/ 90, /*durationUs*/ 1000 * 1000);

		REQUIRE(!alrDetector.GetAlrStartTimeUs().has_value());

		// Dropping to a fifth of it is not.
		sendTraffic(alrDetector, /*usagePercentage*/ 20, /*durationUs*/ 1500 * 1000);

		REQUIRE(alrDetector.GetAlrStartTimeUs().has_value());

		// And going back to using the whole estimate ends it.
		sendTraffic(alrDetector, /*usagePercentage*/ 100, /*durationUs*/ 4000 * 1000);

		REQUIRE(!alrDetector.GetAlrStartTimeUs().has_value());
	}

	SECTION("a short burst doesn't end the state")
	{
		RTC::BWE::AlrDetector alrDetector(std::addressof(shared));

		alrDetector.SetEstimatedBitrate(EstimatedBitrate);

		REQUIRE(!alrDetector.GetAlrStartTimeUs().has_value());

		sendTraffic(alrDetector, /*usagePercentage*/ 20, /*durationUs*/ 1000 * 1000);

		REQUIRE(alrDetector.GetAlrStartTimeUs().has_value());

		// Sending half again as much as the estimate allows, but only briefly, eats
		// into the budget without emptying it.
		sendTraffic(alrDetector, /*usagePercentage*/ 150, /*durationUs*/ 100 * 1000);

		REQUIRE(alrDetector.GetAlrStartTimeUs().has_value());

		// Sustained traffic is what ends it.
		sendTraffic(alrDetector, /*usagePercentage*/ 100, /*durationUs*/ 3000 * 1000);

		REQUIRE(!alrDetector.GetAlrStartTimeUs().has_value());
	}

	SECTION("a lower estimate keeps the state until the same traffic fills the link")
	{
		RTC::BWE::AlrDetector alrDetector(std::addressof(shared));

		alrDetector.SetEstimatedBitrate(EstimatedBitrate);

		REQUIRE(!alrDetector.GetAlrStartTimeUs().has_value());

		sendTraffic(alrDetector, /*usagePercentage*/ 20, /*durationUs*/ 1000 * 1000);

		REQUIRE(alrDetector.GetAlrStartTimeUs().has_value());

		// An estimate falling doesn't end the state by itself, so that whoever
		// decides to probe still sees it and can react to the drop.
		alrDetector.SetEstimatedBitrate(EstimatedBitrate / 5);

		REQUIRE(alrDetector.GetAlrStartTimeUs().has_value());

		// It ends shortly after, because the very same traffic now fills a link
		// that is believed to be five times smaller.
		sendTraffic(alrDetector, /*usagePercentage*/ 50, /*durationUs*/ 1000 * 1000);

		REQUIRE(!alrDetector.GetAlrStartTimeUs().has_value());
	}
}
