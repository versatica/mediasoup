#include "common.hpp"
#include "RTC/RateCalculator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits> // std::numeric_limits
#include <vector>

SCENARIO("RateCalculator", "[rate-calculator]")
{
	struct TestRateCalculatorData
	{
		int64_t offset;
		uint32_t size;
		uint32_t rate;
	};

	auto validate =
	  [](RTC::RateCalculator& rate, uint64_t timeBaseMs, const std::vector<TestRateCalculatorData>& input)
	{
		for (const auto& item : input)
		{
			rate.Update(item.size, timeBaseMs + item.offset);

			REQUIRE(rate.GetRate(timeBaseMs + item.offset) == item.rate);
		}

		// Repeat forcing nowMs to be 0.
		rate.Reset();

		for (const auto& item : input)
		{
			rate.Update(item.size, timeBaseMs + item.offset);

			REQUIRE(rate.GetRate(0 + item.offset) == item.rate);
		}

		// Repeat forcing nowMs to be std::numeric_limits<uint64_t>::max() - 100.
		rate.Reset();

		for (const auto& item : input)
		{
			rate.Update(item.size, timeBaseMs + item.offset);

			REQUIRE(rate.GetRate(std::numeric_limits<uint64_t>::max() - 100 + item.offset) == item.rate);
		}
	};

	const uint64_t nowMs = 12345678;

	SECTION("receive single item per 1000 ms")
	{
		RTC::RateCalculator rate;

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0, .size=5, .rate=40 }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("receive multiple items per 1000 ms")
	{
		RTC::RateCalculator rate;

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0,   .size=5, .rate=40  },
			{ .offset=100, .size=2, .rate=56  },
			{ .offset=300, .size=2, .rate=72  },
			{ .offset=999, .size=4, .rate=104 }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("receive item every 1000 ms")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0,    .size=5, .rate=40 },
			{ .offset=1000, .size=5, .rate=40 },
			{ .offset=2000, .size=5, .rate=40 }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("slide")
	{
		RTC::RateCalculator rate(1000, 8000, 1000);

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0,    .size=5, .rate=40 },
			{ .offset=999,  .size=2, .rate=56 },
			{ .offset=1001, .size=1, .rate=24 },
			{ .offset=1001, .size=1, .rate=32 },
			{ .offset=2000, .size=1, .rate=24 }
		};
		// clang-format on

		validate(rate, nowMs, input);

		REQUIRE(rate.GetRate(nowMs + 3001) == 0);
	}

	SECTION("slide with 100 items")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0,    .size=5, .rate=40 },
			{ .offset=999,  .size=2, .rate=56 },
			{ .offset=1001, .size=1, .rate=24 }, // merged inside 999
			{ .offset=1001, .size=1, .rate=32 }, // merged inside 999
			{ .offset=2000, .size=1, .rate=8  }  // it will erase the item with
			                // timestamp=999, removing also the next two samples. The
			                // end estimation will include only the last sample.
		};
		// clang-format on

		validate(rate, nowMs, input);

		REQUIRE(rate.GetRate(nowMs + 3001) == 0);
	}

	SECTION("wrap")
	{
		// window: 1000ms, items: 5 (granularity: 200ms)
		RTC::RateCalculator rate(1000, 8000, 5);

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=1000, .size=1, .rate=1*8 },
			{ .offset=1200, .size=1, .rate=(1*8) + (1*8) },
			{ .offset=1400, .size=1, .rate=(1*8) + (2*8) },
			{ .offset=1600, .size=1, .rate=(1*8) + (3*8) },
			{ .offset=1800, .size=1, .rate=(1*8) + (4*8) },
			{ .offset=2000, .size=1, .rate=(1*8) + ((5-1)*8) }, // starts wrap here
			{ .offset=2200, .size=1, .rate=(1*8) + ((6-2)*8) },
			{ .offset=2400, .size=1, .rate=(1*8) + ((7-3)*8) },
			{ .offset=2600, .size=1, .rate=(1*8) + ((8-4)*8) },
			{ .offset=2800, .size=1, .rate=(1*8) + ((9-5)*8) },
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	// NOTE: This test reproduces a crash (now fixed):
	//   https://github.com/versatica/mediasoup/issues/1316
	SECTION("buffer overflow should not crash")
	{
		// window: 1000ms, items: 3 (granularity: 334ms)
		RTC::RateCalculator rate(1000, 8000, 3);

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0,   .size=1, .rate=8  },
			{ .offset=333, .size=1, .rate=16 },
			{ .offset=666, .size=1, .rate=24 },
			{ .offset=999, .size=1, .rate=32 },
  	};
		// clang-format on

		validate(rate, nowMs, input);
	}

	// NOTE: This pins the item grid alignment. Items must advance by whole
	// itemSizeMs steps so that a full ring always spans the window size. If the
	// newest item start time jumped to nowMs instead, items would absorb the
	// elapsed time remainder, the ring would span more time than the window, and
	// the rate would be over-reported.
	SECTION("item boundaries do not drift with traffic timing")
	{
		// window: 1000ms, items: 100 (granularity: 10ms)
		RTC::RateCalculator rate(1000, 8000, 100);

		// 11ms spacing, deliberately not a multiple of the 10ms granularity.
		for (uint64_t i{ 0 }; i <= 100; ++i)
		{
			rate.Update(1, nowMs + (i * 11));
		}

		// The ring spans the items starting at [110, 1100], which hold the 91
		// packets sent at 110, 121 ... 1100.
		REQUIRE(rate.GetRate(nowMs + 1100) == 91 * 8);
	}

	// NOTE: This pins the GetRate() memoization key, which is both nowMs and the
	// total count. Keying it on nowMs alone would return a stale rate.
	SECTION("rate is recalculated after Update() with the same now")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		rate.Update(5, nowMs);

		REQUIRE(rate.GetRate(nowMs) == 40);

		rate.Update(5, nowMs);

		REQUIRE(rate.GetRate(nowMs) == 80);

		rate.Update(5, nowMs);

		REQUIRE(rate.GetRate(nowMs) == 120);

		// Repeated reads with no Update() in between must be stable.
		REQUIRE(rate.GetRate(nowMs) == 120);
		REQUIRE(rate.GetRate(nowMs) == 120);
	}

	// NOTE: This pins the item size rounding for a window size which is not a
	// multiple of it. Rounding the item size down would make a full ring span more
	// time than the window, over-reporting the rate.
	SECTION("window not divisible by items spans the window size")
	{
		// window: 1000ms, items: 3 (granularity: 334ms)
		RTC::RateCalculator rate(1000, 8000, 3);

		// Feed way past the ring size, so that any extra span accumulates.
		for (uint64_t i{ 0 }; i < 100; ++i)
		{
			rate.Update(1, nowMs + (i * 334));
		}

		// Steady state is a full ring of 3 items holding 1 byte each.
		REQUIRE(rate.GetRate(nowMs + (99 * 334)) == 24);
	}

	// NOTE: This pins that the GetRate() memoization needs no "not calculated yet"
	// mark. Its zeroed initial state is a valid entry, so a read at time 0 must be
	// neither a stale hit nor a miss returning something else than 0.
	SECTION("rate at time 0 on a fresh and on a reset calculator")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		REQUIRE(rate.GetRate(0) == 0);

		rate.Update(5, 0);

		REQUIRE(rate.GetRate(0) == 40);

		rate.Reset();

		REQUIRE(rate.GetRate(0) == 0);

		rate.Update(5, 0);

		REQUIRE(rate.GetRate(0) == 40);
	}

	// NOTE: This pins the constructor clamping. A zero number of items used to
	// divide by zero, and a zero window size to leave an empty buffer.
	SECTION("degenerate constructor arguments are clamped")
	{
		RTC::RateCalculator noItems(1000, 8000, 0);
		RTC::RateCalculator oneItem(1000, 8000, 1);
		RTC::RateCalculator noWindow(0, 8000, 100);

		noItems.Update(5, nowMs);
		oneItem.Update(5, nowMs);
		noWindow.Update(5, nowMs);

		REQUIRE(noItems.GetRate(nowMs) == 40);
		REQUIRE(oneItem.GetRate(nowMs) == 40);
		// The window size is clamped to 1ms.
		REQUIRE(noWindow.GetRate(nowMs) == 40000);
	}
}
