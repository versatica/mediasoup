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
		std::optional<int64_t> rate;
	};

	auto validate =
	  [](RTC::RateCalculator& rate, int64_t timeBaseMs, const std::vector<TestRateCalculatorData>& input)
	{
		for (const auto& item : input)
		{
			rate.Update(item.size, timeBaseMs + item.offset);

			REQUIRE(rate.GetRate(timeBaseMs + item.offset) == item.rate);
		}

		// Repeat asking for the rate at a time older than the whole window. There is
		// no period between the samples and a time that precedes all of them, so
		// there is nothing to measure.
		rate.Reset();

		for (const auto& item : input)
		{
			rate.Update(item.size, timeBaseMs + item.offset);

			REQUIRE(rate.GetRate(item.offset) == std::nullopt);
		}

		// The reads above left the data untouched, so the rate at the time of the
		// latest sample is still the one that was reported for it.
		REQUIRE(rate.GetRate(timeBaseMs + input.back().offset) == input.back().rate);

		// Asking for the rate far in the future expires every item.
		REQUIRE(rate.GetRate(std::numeric_limits<int64_t>::max()) == std::nullopt);
	};

	const int64_t nowMs = 12345678;

	// NOTE: This pins the three states in which there is nothing to measure, and
	// that a lone sample does become a measurement once the window has filled
	// around it.
	SECTION("no rate until there is something to measure")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		// Not a single sample.
		REQUIRE(rate.GetRate(nowMs) == std::nullopt);

		rate.Update(5, nowMs);

		// A single sample taken at an instant says how much data there was, not how
		// fast it is flowing.
		REQUIRE(rate.GetRate(nowMs) == std::nullopt);
		REQUIRE(rate.GetRate(nowMs + 500) == std::nullopt);

		// Once the window has filled, that lone sample is all the data there was
		// during a whole window, which is a rate.
		REQUIRE(rate.GetRate(nowMs + 999) == 40);

		// And one more item later it has expired, leaving nothing again.
		REQUIRE(rate.GetRate(nowMs + 1000) == std::nullopt);
	}

	// NOTE: This pins the headline of the whole thing: the divisor is the period
	// the samples span, not the window they sit in. Dividing by the window here
	// would report 32 bps for a link carrying 16 kbps.
	SECTION("rate is measured over the period the samples span")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		rate.Update(1000, nowMs);
		rate.Update(1000, nowMs + 999);

		REQUIRE(rate.GetRate(nowMs + 999) == 16000);
	}

	SECTION("receive single item per 1000 ms")
	{
		RTC::RateCalculator rate;

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0, .size=5, .rate=std::nullopt }
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
			{ .offset=0,   .size=5, .rate=std::nullopt },
			{ .offset=100, .size=2, .rate=554          },
			{ .offset=300, .size=2, .rate=239          },
			{ .offset=999, .size=4, .rate=104          }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("receive item every 1000 ms")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		// Each sample expires the previous one, so there is never more than a lone
		// sample within a window that has just been emptied.
		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0,    .size=5, .rate=std::nullopt },
			{ .offset=1000, .size=5, .rate=std::nullopt },
			{ .offset=2000, .size=5, .rate=std::nullopt }
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
			{ .offset=0,    .size=5, .rate=std::nullopt },
			{ .offset=999,  .size=2, .rate=56           },
			{ .offset=1001, .size=1, .rate=24           },
			{ .offset=1001, .size=1, .rate=32           },
			{ .offset=2000, .size=1, .rate=24           }
		};
		// clang-format on

		validate(rate, nowMs, input);

		REQUIRE(rate.GetRate(nowMs + 3001) == std::nullopt);
	}

	SECTION("slide with 100 items")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0,    .size=5, .rate=std::nullopt },
			{ .offset=999,  .size=2, .rate=56           },
			{ .offset=1001, .size=1, .rate=24           }, // merged inside 999
			{ .offset=1001, .size=1, .rate=32           }, // merged inside 999
			{ .offset=2000, .size=1, .rate=std::nullopt }  // it will erase the item
			                // with timestamp=999, removing also the next two samples.
			                // Only the last sample is left, and its period is a single
			                // millisecond.
		};
		// clang-format on

		validate(rate, nowMs, input);

		REQUIRE(rate.GetRate(nowMs + 3001) == std::nullopt);
	}

	SECTION("wrap")
	{
		// window: 1000ms, items: 5 (granularity: 200ms)
		RTC::RateCalculator rate(1000, 8000, 5);

		// The first five samples are measured over the period they span, which is
		// still shorter than the window, so they do not add up in steps of 8.
		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=1000, .size=1, .rate=std::nullopt         },
			{ .offset=1200, .size=1, .rate=80                   },
			{ .offset=1400, .size=1, .rate=60                   },
			{ .offset=1600, .size=1, .rate=53                   },
			{ .offset=1800, .size=1, .rate=50                   },
			{ .offset=2000, .size=1, .rate=(1*8) + ((5-1)*8)    }, // starts wrap here
			{ .offset=2200, .size=1, .rate=(1*8) + ((6-2)*8)    },
			{ .offset=2400, .size=1, .rate=(1*8) + ((7-3)*8)    },
			{ .offset=2600, .size=1, .rate=(1*8) + ((8-4)*8)    },
			{ .offset=2800, .size=1, .rate=(1*8) + ((9-5)*8)    },
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
			{ .offset=0,   .size=1, .rate=std::nullopt },
			{ .offset=333, .size=1, .rate=48           },
			{ .offset=666, .size=1, .rate=36           },
			{ .offset=999, .size=1, .rate=32           },
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
		for (int64_t i{ 0 }; i <= 100; ++i)
		{
			rate.Update(1, nowMs + (i * 11));
		}

		// The ring spans the items starting at [110, 1100], which hold the 91
		// packets sent at 110, 121 ... 1100.
		REQUIRE(rate.GetRate(nowMs + 1100) == 91 * 8);
	}

	// NOTE: This pins the GetRate() memoization key, which is the time plus both
	// totals. Keying it on the time alone would return a stale rate.
	SECTION("rate is recalculated after Update() with the same now")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		// Two samples apart in time, so that there is a period to measure over and
		// the reads below are not rejected for lack of one.
		rate.Update(5, nowMs);
		rate.Update(5, nowMs + 500);

		REQUIRE(rate.GetRate(nowMs + 500) == 160);

		rate.Update(5, nowMs + 500);

		REQUIRE(rate.GetRate(nowMs + 500) == 240);

		rate.Update(5, nowMs + 500);

		REQUIRE(rate.GetRate(nowMs + 500) == 319);

		// Repeated reads with no Update() in between must be stable.
		REQUIRE(rate.GetRate(nowMs + 500) == 319);
		REQUIRE(rate.GetRate(nowMs + 500) == 319);
	}

	// NOTE: This pins the item size rounding for a window size which is not a
	// multiple of it. Rounding the item size down would make a full ring span more
	// time than the window, over-reporting the rate.
	SECTION("window not divisible by items spans the window size")
	{
		// window: 1000ms, items: 3 (granularity: 334ms)
		RTC::RateCalculator rate(1000, 8000, 3);

		// Feed way past the ring size, so that any extra span accumulates.
		for (int64_t i{ 0 }; i < 100; ++i)
		{
			rate.Update(1, nowMs + (i * 334));
		}

		// Steady state is a full ring of 3 items holding 1 byte each.
		REQUIRE(rate.GetRate(nowMs + (99 * 334)) == 24);
	}

	// NOTE: This pins that the GetRate() memoization needs no "not calculated yet"
	// mark. Its zeroed initial state is a valid entry, so a read at time 0 must be
	// neither a stale hit nor a miss returning something else than no rate at all.
	SECTION("rate at time 0 on a fresh and on a reset calculator")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		REQUIRE(rate.GetRate(0) == std::nullopt);

		rate.Update(5, 0);
		rate.Update(5, 500);

		REQUIRE(rate.GetRate(500) == 160);

		rate.Reset();

		REQUIRE(rate.GetRate(0) == std::nullopt);

		rate.Update(5, 0);
		rate.Update(5, 500);

		REQUIRE(rate.GetRate(500) == 160);
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

		noItems.Update(5, nowMs + 500);
		oneItem.Update(5, nowMs + 500);
		noWindow.Update(5, nowMs + 500);

		REQUIRE(noItems.GetRate(nowMs + 500) == 160);
		REQUIRE(oneItem.GetRate(nowMs + 500) == 160);
		// The window size is clamped to 1ms, which leaves every sample alone in a
		// period of a single millisecond, so such a window can never measure a rate.
		REQUIRE(noWindow.GetRate(nowMs + 500) == std::nullopt);
	}
}
