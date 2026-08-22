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
		// window: 1000ms, items: 3 (granularity: 333ms)
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
}
