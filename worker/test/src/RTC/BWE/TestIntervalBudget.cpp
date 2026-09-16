#include "common.hpp"
#include "RTC/BWE/IntervalBudget.hpp"
#include <catch2/catch_test_macros.hpp>

SCENARIO("BWE IntervalBudget", "[bwe][intervalbudget]")
{
	// Span of time the budget is measured over.
	constexpr int64_t WindowUs{ 500 * 1000 };
	constexpr int64_t TargetBitrate{ 100000 };

	// Bytes the given bitrate delivers over the given span of time.
	const auto bitrateToBytes = [](int64_t bitrate, int64_t durationUs) -> int64_t
	{
		return (bitrate * durationUs) / (8 * 1000000);
	};

	SECTION("a new budget holds nothing")
	{
		const RTC::BWE::IntervalBudget intervalBudget(TargetBitrate);

		REQUIRE(intervalBudget.GetBudgetRatio() == 0.0);
		REQUIRE(intervalBudget.GetBytesRemaining() == 0);
	}

	SECTION("time passing without spending fills the budget in proportion")
	{
		constexpr int64_t DeltaTimeUs{ 50 * 1000 };

		RTC::BWE::IntervalBudget intervalBudget(TargetBitrate);

		intervalBudget.IncreaseBudget(DeltaTimeUs);

		// A tenth of the window went by, so a tenth of it is what the budget holds.
		REQUIRE(intervalBudget.GetBudgetRatio() == 0.1);
		REQUIRE(intervalBudget.GetBytesRemaining() == bitrateToBytes(TargetBitrate, DeltaTimeUs));
	}

	SECTION("the budget doesn't grow past the window")
	{
		RTC::BWE::IntervalBudget intervalBudget(TargetBitrate);

		intervalBudget.IncreaseBudget(/*deltaTimeUs*/ 1000 * 1000);

		REQUIRE(intervalBudget.GetBudgetRatio() == 1.0);
		REQUIRE(intervalBudget.GetBytesRemaining() == bitrateToBytes(TargetBitrate, WindowUs));
	}

	SECTION("lowering the bitrate brings the budget down to the new window")
	{
		RTC::BWE::IntervalBudget intervalBudget(TargetBitrate);

		intervalBudget.IncreaseBudget(/*deltaTimeUs*/ WindowUs / 2);
		intervalBudget.SetTargetBitrate(TargetBitrate / 10);

		// Half of the old window is more than the whole of the new one.
		REQUIRE(intervalBudget.GetBudgetRatio() == 1.0);
		REQUIRE(intervalBudget.GetBytesRemaining() == bitrateToBytes(TargetBitrate / 10, WindowUs));
	}

	SECTION("raising the bitrate leaves the budget where it was")
	{
		RTC::BWE::IntervalBudget intervalBudget(TargetBitrate);

		intervalBudget.IncreaseBudget(/*deltaTimeUs*/ WindowUs);
		intervalBudget.SetTargetBitrate(TargetBitrate * 2);

		// The bytes held are the same ones, but now they're half of the window.
		REQUIRE(intervalBudget.GetBudgetRatio() == 0.5);
		REQUIRE(intervalBudget.GetBytesRemaining() == bitrateToBytes(TargetBitrate, WindowUs));
	}

	SECTION("spending more than there is takes the budget into debt")
	{
		constexpr int64_t OveruseTimeUs{ 50 * 1000 };

		RTC::BWE::IntervalBudget intervalBudget(TargetBitrate);

		intervalBudget.UseBudget(bitrateToBytes(TargetBitrate, OveruseTimeUs));

		// A tenth of the window was spent without any of it having been refilled.
		REQUIRE(intervalBudget.GetBudgetRatio() == -0.1);
		REQUIRE(intervalBudget.GetBytesRemaining() == 0);
	}

	SECTION("the debt doesn't grow past the window either")
	{
		RTC::BWE::IntervalBudget intervalBudget(TargetBitrate);

		intervalBudget.UseBudget(bitrateToBytes(TargetBitrate, /*durationUs*/ 1000 * 1000));

		REQUIRE(intervalBudget.GetBudgetRatio() == -1.0);
		REQUIRE(intervalBudget.GetBytesRemaining() == 0);
	}

	SECTION("what an interval leaves unspent adds up when told to")
	{
		constexpr int64_t DeltaTimeUs{ 50 * 1000 };

		RTC::BWE::IntervalBudget intervalBudget(TargetBitrate, /*canBuildUpUnderuse*/ true);

		intervalBudget.IncreaseBudget(DeltaTimeUs);

		REQUIRE(intervalBudget.GetBudgetRatio() == 0.1);
		REQUIRE(intervalBudget.GetBytesRemaining() == bitrateToBytes(TargetBitrate, DeltaTimeUs));

		intervalBudget.IncreaseBudget(DeltaTimeUs);

		REQUIRE(intervalBudget.GetBudgetRatio() == 0.2);
		REQUIRE(intervalBudget.GetBytesRemaining() == bitrateToBytes(TargetBitrate, 2 * DeltaTimeUs));
	}

	SECTION("and doesn't when told not to")
	{
		constexpr int64_t DeltaTimeUs{ 50 * 1000 };

		RTC::BWE::IntervalBudget intervalBudget(TargetBitrate, /*canBuildUpUnderuse*/ false);

		intervalBudget.IncreaseBudget(DeltaTimeUs);

		REQUIRE(intervalBudget.GetBudgetRatio() == 0.1);
		REQUIRE(intervalBudget.GetBytesRemaining() == bitrateToBytes(TargetBitrate, DeltaTimeUs));

		intervalBudget.IncreaseBudget(DeltaTimeUs);

		// The second span refills just as much as the first one did, instead of
		// adding to what was already there.
		REQUIRE(intervalBudget.GetBudgetRatio() == 0.1);
		REQUIRE(intervalBudget.GetBytesRemaining() == bitrateToBytes(TargetBitrate, DeltaTimeUs));
	}
}
