#include "common.hpp"
#include "RTC/SeqManager.hpp"
#include <catch.hpp>
#include <vector>

using namespace RTC;

struct TestSeqManagerInput
{
	TestSeqManagerInput(
	  uint16_t input, uint16_t output, bool sync = false, bool drop = false, uint16_t offset = 0)
	  : input(input), output(output), sync(sync), drop(drop), offset(offset)
	{
	}

	uint16_t input{ 0 };
	uint16_t output{ 0 };
	bool sync{ false };
	bool drop{ false };
	uint16_t offset{ 0 };
};

void validate(SeqManager<uint16_t>& seqManager, std::vector<TestSeqManagerInput>& inputs)
{
	for (auto& element : inputs)
	{
		if (element.sync)
			seqManager.Sync(element.input - 1);

		if (element.offset)
			seqManager.Offset(element.offset);

		if (element.drop)
		{
			seqManager.Drop(element.input);
		}
		else
		{
			uint16_t output;

			seqManager.Input(element.input, output);
			REQUIRE(output == element.output);
		}
	}
}

SCENARIO("SeqManager", "[rtc]")
{
	SECTION("0 is greater than 65000")
	{
		REQUIRE(SeqManager<uint16_t>::IsSeqHigherThan(0, 65000) == true);
	}

	SECTION("receive ordered numbers, no sync, no drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{  0,  0, false, false },
			{  1,  1, false, false },
			{  2,  2, false, false },
			{  3,  3, false, false },
			{  4,  4, false, false },
			{  5,  5, false, false },
			{  6,  6, false, false },
			{  7,  7, false, false },
			{  8,  8, false, false },
			{  9,  9, false, false },
			{ 10, 10, false, false },
			{ 11, 11, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive ordered numbers, sync, no drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{  0,  0, false, false },
			{  1,  1, false, false },
			{  2,  2, false, false },
			{ 80,  3, true, false },
			{ 81,  4, false, false },
			{ 82,  5, false, false },
			{ 83,  6, false, false },
			{ 84,  7, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive ordered numbers, sync, drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{  0,  0, false, false },
			{  1,  1, false, false },
			{  2,  2, false, false },
			{  3,  3, false, false },
			{  4,  4,  true, false }, // sync.
			{  5,  5, false, false },
			{  6,  6, false, false },
			{  7,  7,  true, false }, // sync.
			{  8,  0, false,  true }, // drop.
			{  9,  8, false, false },
			{ 11,  0, false,  true }, // drop.
			{ 10,  9, false, false },
			{ 12, 10, false, false },
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive ordered wrapped numbers")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{ 65533,  65533, false, false },
			{ 65534,  65534, false, false },
			{ 65535,  65535, false, false },
			{     0,      0, false, false },
			{     1,      1, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive sequence numbers with a big jump")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{    0,   0, false, false },
			{    1,   1, false, false },
			{ 1000, 1000, false, false },
			{ 1001, 1001, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive mixed numbers with a big jump, drop before jump")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{   0,   0, false, false },
			{   1,   0, false,  true }, // drop.
			{ 100,  99, false, false },
			{ 100,  99, false, false },
			{ 103,   0, false,  true }, // drop.
			{ 101, 100, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive mixed numbers with a big jump, drop after jump")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{   0,   0, false, false },
			{   1,   1, false, false },
			{ 100,   0, false,  true }, // drop.
			{ 103,   0, false,  true }, // drop.
			{ 101, 100, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("drop, receive numbers newer and older than the one dropped")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{ 0,  0, false, false },
			{ 2,  0, false,  true }, // drop.
			{ 3,  2, false, false },
			{ 4,  3, false, false },
			{ 1,  1, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive mixed numbers, sync, drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{     0,  0, false, false },
			{     1,  1, false, false },
			{     2,  2, false, false },
			{     3,  3, false, false },
			{     7,  7, false, false },
			{     6,  0, false,  true }, // drop.
			{     8,  8, false, false },
			{    10, 10, false, false },
			{     9,  9, false, false },
			{    11, 11, false, false },
			{     0, 12,  true, false }, // sync.
			{     2, 14, false, false },
			{     3, 15, false, false },
			{     4, 16, false, false },
			{     5, 17, false, false },
			{     6, 18, false, false },
			{     7, 19, false, false },
			{     8, 20, false, false },
			{     9, 21, false, false },
			{    10, 22, false, false },
			{     9,  0, false,  true }, // drop.
			{    61, 23,  true, false }, // sync.
			{    62, 24, false, false },
			{    63, 25, false, false },
			{    64, 26, false, false },
			{    65, 27, false, false },
			{    11, 28,  true, false }, // sync.
			{    12, 29, false, false },
			{    13, 30, false, false },
			{    14, 31, false, false },
			{    15, 32, false, false },
			{     1, 33,  true, false }, // sync.
			{     2, 34, false, false },
			{     3, 35, false, false },
			{     4, 36, false, false },
			{     5, 37, false, false },
			{ 65533, 38,  true, false }, // sync.
			{ 65534, 39, false, false },
			{ 65535, 40, false, false },
			{     0, 41,  true, false }, // sync.
			{     1, 42, false, false },
			{     3,  0, false,  true }, // drop.
			{     4, 44, false, false },
			{     5, 45, false, false },
			{     6, 46, false, false },
			{     7, 47, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive ordered numbers, sync, no drop, increase input")
	{
		// clang-format off
		std::vector<TestSeqManagerInput> inputs =
		{
			{  0,   0, false, false     },
			{  1,   1, false, false     },
			{  2,   2, false, false     },
			{ 80,  23,  true, false, 20 },
			{ 81,  24, false, false     },
			{ 82,  25, false, false     },
			{ 83,  26, false, false     },
			{ 84,  27, false, false     }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}
}
