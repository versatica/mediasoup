#include "common.hpp"
#include "DepLibUV.hpp"
#include "RTC/RateCalculator.hpp"
#include <catch.hpp>
#include <vector>

using namespace RTC;

struct data
{
	uint32_t offset;
	uint32_t size;
	uint32_t rate;
};

void validate(RateCalculator& rate, uint64_t timeBase, std::vector<data>& input)
{
	for (auto it = input.begin(); it != input.end(); ++it)
	{
		auto& item = *it;

		rate.Update(item.size, timeBase + item.offset);
		REQUIRE(rate.GetRate(timeBase + item.offset) == item.rate);
	}
}

SCENARIO("Bitrate calculator", "[rtp][bitrate]")
{
	uint64_t nowMs = DepLibUV::GetTimeMs();

	SECTION("receive single item per 1000 ms")
	{
		RateCalculator rate;

		// clang-format off
		std::vector<data> input =
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
		std::vector<data> input =
		{
			{ 0,   5, 40  },
			{ 100, 2, 56  },
			{ 300, 2, 72  },
			{ 999, 4, 104 }
		};
		// clang-format on

		validate(rate, nowMs, input);
	}

	SECTION("slide")
	{
		RateCalculator rate;

		// clang-format off
		std::vector<data> input =
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

	SECTION("wrap")
	{
		RateCalculator rate(1000, 8000, 5);

		// clang-format off
		std::vector<data> input =
		{
			{ 1000, 1, 1*8 },
			{ 1001, 1, 1*8 + 1*8 },
			{ 1002, 1, 1*8 + 2*8 },
			{ 1003, 1, 1*8 + 3*8 },
			{ 1004, 1, 1*8 + 4*8 },
			{ 1005, 1, 1*8 + (5-1)*8 }, // starts wrap here
			{ 1006, 1, 1*8 + (6-2)*8 },
			{ 1007, 1, 1*8 + (7-3)*8 },
			{ 1008, 1, 1*8 + (8-4)*8 },
			{ 1009, 1, 1*8 + (9-5)*8 },
		};
		// clang-format on

		validate(rate, nowMs, input);
	}
}
