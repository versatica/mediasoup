#include "common.hpp"
#include "RTC/RateCalculator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib> // std::abs()
#include <limits>  // std::numeric_limits
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

		// The period is the whole window, so both samples count: 2000 * 8000 / 1000.
		REQUIRE(rate.GetRate(nowMs + 999) == 16000);
	}

	// NOTE: The five sections below use a window of 500 ms split into items of a
	// single millisecond, so that expiration happens sample by sample and every
	// expected value can be worked out by hand.

	SECTION("a lone sample is measured once the window has filled around it")
	{
		// window: 500ms, items: 500 (granularity: 1ms)
		RTC::RateCalculator rate(500, 8000, 500);

		REQUIRE(rate.GetRate(nowMs) == std::nullopt);

		// One byte per millisecond, given as a single sample.
		rate.Update(500, nowMs);

		// Only one sample, and the window has not filled.
		REQUIRE(rate.GetRate(nowMs + 498) == std::nullopt);

		// Now it has, so the single sample is all the data of a whole window.
		REQUIRE(rate.GetRate(nowMs + 499) == 1000 * 8);

		// Another one on top doubles it.
		rate.Update(500, nowMs + 499);

		REQUIRE(rate.GetRate(nowMs + 499) == 2 * 1000 * 8);

		// And now the first one drops out.
		REQUIRE(rate.GetRate(nowMs + 500) == 1000 * 8);
	}

	SECTION("samples of size zero are measured as no data rather than no rate")
	{
		// window: 500ms, items: 500 (granularity: 1ms)
		RTC::RateCalculator rate(500, 8000, 500);

		REQUIRE(rate.GetRate(nowMs) == std::nullopt);

		rate.Update(500, nowMs);
		rate.Update(0, nowMs + 499);

		REQUIRE(rate.GetRate(nowMs + 499) == 1000 * 8);

		// The first sample drops out, leaving the empty one, which is a rate of
		// zero rather than no rate at all.
		REQUIRE(rate.GetRate(nowMs + 500) == 0);

		// And now the empty one drops out too.
		REQUIRE(rate.GetRate(nowMs + 1000) == std::nullopt);
	}

	SECTION("a quiet period is measured as no data until it empties the window")
	{
		// window: 500ms, items: 500 (granularity: 1ms)
		RTC::RateCalculator rate(500, 8000, 500);

		REQUIRE(rate.GetRate(nowMs) == std::nullopt);

		rate.Update(0, nowMs);

		REQUIRE(rate.GetRate(nowMs + 499) == 0);

		// Move the window along so that the sample falls out.
		REQUIRE(rate.GetRate(nowMs + 500) == std::nullopt);

		// Move it a long way out, which makes the next sample start a new period.
		rate.Update(0, nowMs + 1500);

		REQUIRE(rate.GetRate(nowMs + 1500) == std::nullopt);

		// The second one gives a period to measure over.
		rate.Update(0, nowMs + 1501);

		REQUIRE(rate.GetRate(nowMs + 1501) == 0);
	}

	SECTION("silence longer than the window starts a new period")
	{
		// window: 500ms, items: 500 (granularity: 1ms)
		RTC::RateCalculator rate(500, 8000, 500);

		// 1000 bytes per millisecond until the window has filled, which must not make
		// the estimation error grow as the measured period is extended.
		int64_t prevError{ 8000000 };
		std::optional<int64_t> bitrate;

		for (int64_t i{ 1 }; i < 10000; ++i)
		{
			rate.Update(1000, nowMs + i);

			bitrate = rate.GetRate(nowMs + i);

			if (bitrate.has_value())
			{
				const int64_t error = std::abs(8000000 - bitrate.value());

				REQUIRE(error <= prevError + 1);

				prevError = error;
			}
		}

		REQUIRE(bitrate == 8000000);

		// Silence over the window size, read during the silence.
		REQUIRE(rate.GetRate(nowMs + 10500) == std::nullopt);

		// So the samples that come next are measured over their own period, which is
		// the 2ms they span, leaving out the one that starts it: 1000 * 8000 / 2.
		rate.Update(1000, nowMs + 10500);
		rate.Update(1000, nowMs + 10501);

		REQUIRE(rate.GetRate(nowMs + 10501) == 4000000);

		// A manual reset does the same.
		rate.Reset();

		REQUIRE(rate.GetRate(nowMs + 10501) == std::nullopt);

		rate.Update(1000, nowMs + 10501);
		rate.Update(1000, nowMs + 10502);

		REQUIRE(rate.GetRate(nowMs + 10502) == 4000000);
	}

	// NOTE: This is what the margin exists for: the samples come a whole window
	// apart, so without it each of them would find the window empty, restart the
	// measured period and never be measured at all.
	SECTION("a stream sending as often as the window is still measured")
	{
		// window: 1000ms, items: 1000 (granularity: 1ms), counting frames
		RTC::RateCalculator rate(1000, 1000, 1000);

		REQUIRE(rate.GetRate(nowMs) == std::nullopt);

		// One frame per second for ten seconds.
		for (int64_t i{ 0 }; i < 10000; i += 1000)
		{
			rate.Update(1, nowMs + i);

			if (i > 0)
			{
				REQUIRE(rate.GetRate(nowMs + i) == 1);
			}
		}
	}

	// NOTE: The overflow is provoked through the scale rather than through the size
	// of the samples, since Update() takes a size_t and on a 32 bit build no amount
	// of samples of that size makes the result overflow.
	SECTION("a rate that does not fit is no rate at all")
	{
		// window: 1000ms, items: 1000 (granularity: 1ms)
		RTC::RateCalculator rate(1000, 1e30F, 1000);

		rate.Update(1000, nowMs);
		rate.Update(1000, nowMs + 1);

		REQUIRE(rate.GetRate(nowMs + 1) == std::nullopt);
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
			// A lone sample with the window unfilled.
			{ .offset=0,   .size=5, .rate=std::nullopt },
			// 2 * 8000 / 101, the first sample being the one that starts the period.
			{ .offset=100, .size=2, .rate=158          },
			// (2 + 2) * 8000 / 301.
			{ .offset=300, .size=2, .rate=106          },
			// The period is the whole window now, so every sample counts:
			// (5 + 2 + 2 + 4) * 8000 / 1000.
			{ .offset=999, .size=4, .rate=104          }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("receive item every 1000 ms")
	{
		RTC::RateCalculator rate(1000, 8000, 100);

		// Each sample expires the previous one, but they come close enough together
		// for the measured period to be kept, so each of them is measured over the
		// window instead of over the instant it was taken in.
		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=0,    .size=5, .rate=std::nullopt },
			{ .offset=1000, .size=5, .rate=40           },
			{ .offset=2000, .size=5, .rate=40           }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("slide")
	{
		RTC::RateCalculator rate(1000, 8000, 1000);

		// From the second sample on the period is already the whole window, so every
		// rate below is the bytes left within it times 8.
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
			{ .offset=2000, .size=1, .rate=8            }  // it will erase the item
			                // with timestamp=999, removing also the next two samples.
			                // Only the last sample is left, and it comes close enough
			                // to the previous one to keep being measured over the
			                // window.
		};
		// clang-format on

		validate(rate, nowMs, input);

		REQUIRE(rate.GetRate(nowMs + 3001) == std::nullopt);
	}

	SECTION("wrap")
	{
		// window: 1000ms, items: 5 (granularity: 200ms)
		RTC::RateCalculator rate(1000, 8000, 5);

		// A byte every 200ms is 40 bps, and that is what comes out from the second
		// sample on: leaving out the one that starts the period is what makes the
		// samples and the gaps between them match while the window fills.
		// clang-format off
		const std::vector<TestRateCalculatorData> input =
		{
			{ .offset=1000, .size=1, .rate=std::nullopt         },
			{ .offset=1200, .size=1, .rate=40                   },
			{ .offset=1400, .size=1, .rate=40                   },
			{ .offset=1600, .size=1, .rate=40                   },
			{ .offset=1800, .size=1, .rate=40                   },
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
			// 1 * 8000 / 334, without the sample that starts the period.
			{ .offset=333, .size=1, .rate=24           },
			// 2 * 8000 / 667.
			{ .offset=666, .size=1, .rate=24           },
			// The period is the whole window now: 4 * 8000 / 1000.
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

		// 5 * 8000 / 501, without the sample that starts the period.
		REQUIRE(rate.GetRate(nowMs + 500) == 80);

		rate.Update(5, nowMs + 500);

		// 10 * 8000 / 501.
		REQUIRE(rate.GetRate(nowMs + 500) == 160);

		rate.Update(5, nowMs + 500);

		// 15 * 8000 / 501.
		REQUIRE(rate.GetRate(nowMs + 500) == 240);

		// Repeated reads with no Update() in between must be stable.
		REQUIRE(rate.GetRate(nowMs + 500) == 240);
		REQUIRE(rate.GetRate(nowMs + 500) == 240);
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

		// 5 * 8000 / 501, without the sample that starts the period.
		REQUIRE(rate.GetRate(500) == 80);

		rate.Reset();

		REQUIRE(rate.GetRate(0) == std::nullopt);

		rate.Update(5, 0);
		rate.Update(5, 500);

		// The same as before the reset.
		REQUIRE(rate.GetRate(500) == 80);
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

		// Two samples 500ms apart, of which the one that starts the period is left
		// out: 5 * 8000 / 501.
		REQUIRE(noItems.GetRate(nowMs + 500) == 80);
		REQUIRE(oneItem.GetRate(nowMs + 500) == 80);
		// The window size is clamped to 1ms, which leaves every sample alone in a
		// period of a single millisecond, so such a window can never measure a rate.
		REQUIRE(noWindow.GetRate(nowMs + 500) == std::nullopt);
	}
}
