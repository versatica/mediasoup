#include "include/catch.hpp"
#include "common.h"
#include "RTC/RtpDataCounter.h"
#include "../include/DepLibUV.h"
#include <vector>

using namespace RTC;

struct data {
	uint32_t offset;
	uint32_t size;
	uint32_t rate;
};

void validate(RateCalculator& rate, uint64_t time_base,  std::vector<data>& input)
{
	for (auto it = input.begin(); it != input.end(); ++it)
	{
		auto& item = *it;
		rate.Update(item.size, time_base + item.offset);
		REQUIRE(rate.GetRate(time_base + item.offset) == item.rate);
	}
}

SCENARIO("Bitrate calculator", "[rtp][bitrate]")
{
	DepLibUV::ClassInit();
	uint64_t now = DepLibUV::GetTime();

	SECTION("receive single item per 1000 ms")
	{
		RateCalculator rate;
		std::vector<data> input = {
			{ 0,    5, 40 }
		};

		validate(rate, now, input);
	}

	SECTION("receive multiple items per 1000 ms")
	{
		RateCalculator rate;
		std::vector<data> input = {
			{ 0,    5, 40  },
			{ 100,  2, 56  },
			{ 300,  2, 72  },
			{ 999,  4, 104 }
		};

		validate(rate, now, input);
	}

	SECTION("slide")
	{
		RateCalculator rate;
		std::vector<data> input = {
			{ 0,    5, 40 },
			{ 999,  2, 56 },
			{ 1001, 1, 24 },
			{ 1001, 1, 32 },
			{ 2000, 1, 24 }
		};

		validate(rate, now, input);

		REQUIRE(rate.GetRate(now + 3000) == 0);
	}

	DepLibUV::ClassDestroy();
}
