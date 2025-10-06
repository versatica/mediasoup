#include "common.hpp"
#include "DepLibUV.hpp"
#include "RTC/RateCalculator.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits> // std::numeric_limits
#include <vector>

using namespace RTC;

struct TestRateCalculatorData
{
	int64_t offset;
	uint32_t size;
	uint32_t rate;
};

void validate(RateCalculator& rate, uint64_t timeBase, std::vector<TestRateCalculatorData>& input)
{
	for (auto& item : input)
	{
		rate.Update(item.size, timeBase + item.offset);

		REQUIRE(rate.GetRate(timeBase + item.offset) == item.rate);
	}

	// Repeat forcing nowMs to be 0.
	rate.Reset();

	for (auto& item : input)
	{
		rate.Update(item.size, timeBase + item.offset);

		REQUIRE(rate.GetRate(0 + item.offset) == item.rate);
	}

	// Repeat forcing nowMs to be std::numeric_limits<uint64_t>::max() - 100.
	rate.Reset();

	for (auto& item : input)
	{
		rate.Update(item.size, timeBase + item.offset);

		REQUIRE(rate.GetRate(std::numeric_limits<uint64_t>::max() - 100 + item.offset) == item.rate);
	}
}

SCENARIO("Rate calculator", "[rtp][RateCalculator]")
{
	uint64_t nowMs = DepLibUV::GetTimeMs();

	SECTION("receive single item per 1000 ms")
	{
		RateCalculator rate;

		// clang-format off
		std::vector<TestRateCalculatorData> input =
		{
			{ 0, 5, 40 }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("receive multiple items per 1000 ms")
	{
		RateCalculator rate;

		// clang-format off
		std::vector<TestRateCalculatorData> input =
		{
			{ 0,   5, 40  },
			{ 100, 2, 56  },
			{ 300, 2, 72  },
			{ 999, 4, 104 }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("receive item every 1000 ms")
	{
		RateCalculator rate(1000, 8000, 100);

		// clang-format off
		std::vector<TestRateCalculatorData> input =
		{
			{ 0,    5, 40 },
			{ 1000, 5, 40 },
			{ 2000, 5, 40 }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("slide")
	{
		RateCalculator rate(1000, 8000, 1000);

		// clang-format off
		std::vector<TestRateCalculatorData> input =
		{
			{ 0,    5, 40 },
			{ 999,  2, 56 },
			{ 1001, 1, 24 },
			{ 1001, 1, 32 },
			{ 2000, 1, 24 }
		};
		// clang-format on

		validate(rate, nowMs, input);

		REQUIRE(rate.GetRate(nowMs + 3001) == 0);
	}

	SECTION("slide with 100 items")
	{
		RateCalculator rate(1000, 8000, 100);

		// clang-format off
		std::vector<TestRateCalculatorData> input =
		{
			{ 0,    5, 40 },
			{ 999,  2, 56 },
			{ 1001, 1, 24 }, // merged inside 999
			{ 1001, 1, 32 }, // merged inside 999
			{ 2000, 1, 8 } 	 // it will erase the item with timestamp=999,
							 // removing also the next two samples.
							 // The end estimation will include only the last sample.
		};
		// clang-format on

		validate(rate, nowMs, input);

		REQUIRE(rate.GetRate(nowMs + 3001) == 0);
	}

	SECTION("wrap")
	{
		// window: 1000ms, items: 5 (granularity: 200ms)
		RateCalculator rate(1000, 8000, 5);

		// clang-format off
		std::vector<TestRateCalculatorData> input =
		{
			{ 1000, 1, 1*8 },
			{ 1200, 1, 1*8 + 1*8 },
			{ 1400, 1, 1*8 + 2*8 },
			{ 1600, 1, 1*8 + 3*8 },
			{ 1800, 1, 1*8 + 4*8 },
			{ 2000, 1, 1*8 + (5-1)*8 }, // starts wrap here
			{ 2200, 1, 1*8 + (6-2)*8 },
			{ 2400, 1, 1*8 + (7-3)*8 },
			{ 2600, 1, 1*8 + (8-4)*8 },
			{ 2800, 1, 1*8 + (9-5)*8 },
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	// NOTE: This test reproduces a crash (now fixed):
	//   https://github.com/versatica/mediasoup/issues/1316
	SECTION("buffer overflow should not crash")
	{
		// window: 1000ms, items: 3 (granularity: 333ms)
		RateCalculator rate(1000, 8000, 3);

		// clang-format off
		std::vector<TestRateCalculatorData> input =
		{
			{ 0,   1, 8  },
			{ 333, 1, 16  },
			{ 666, 1, 24  },
			{ 999, 1, 32 },
  	};
		// clang-format on

		validate(rate, nowMs, input);
	}
}
