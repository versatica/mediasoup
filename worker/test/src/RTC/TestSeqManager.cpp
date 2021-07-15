#include "common.hpp"
#include "RTC/SeqManager.hpp"
#include <catch2/catch.hpp>
#include <string>
#include <vector>

using namespace RTC;

template<typename T>
struct TestSeqManagerInput
{
	TestSeqManagerInput(T input, T output, bool sync = false, bool drop = false, T offset = 0)
	  : input(input), output(output), sync(sync), drop(drop), offset(offset)
	{
	}

	T input{ 0 };
	T output{ 0 };
	bool sync{ false };
	bool drop{ false };
	T offset{ 0 };
};

template<typename T>
void validate(SeqManager<T>& seqManager, std::vector<TestSeqManagerInput<T>>& inputs)
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
			T output;

			seqManager.Input(element.input, output);

			// Covert to string because otherwise Catch will print uint8_t as char.
			REQUIRE(std::to_string(output) == std::to_string(element.output));
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
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
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
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{  0, 0, false, false },
			{  1, 1, false, false },
			{  2, 2, false, false },
			{ 80, 3,  true, false },
			{ 81, 4, false, false },
			{ 82, 5, false, false },
			{ 83, 6, false, false },
			{ 84, 7, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive ordered numbers, sync, drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
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
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ 65533, 65533, false, false },
			{ 65534, 65534, false, false },
			{ 65535, 65535, false, false },
			{     0,     0, false, false },
			{     1,     1, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive sequence numbers with a big jump")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{    0,    0, false, false },
			{    1,    1, false, false },
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
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
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
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
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
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ 0, 0, false, false },
			{ 2, 0, false,  true }, // drop.
			{ 3, 2, false, false },
			{ 4, 3, false, false },
			{ 1, 1, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("receive mixed numbers, sync, drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
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
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{  0,  0, false, false     },
			{  1,  1, false, false     },
			{  2,  2, false, false     },
			{ 80, 23,  true, false, 20 },
			{ 81, 24, false, false     },
			{ 82, 25, false, false     },
			{ 83, 26, false, false     },
			{ 84, 27, false, false     }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("drop many inputs at the beginning (using uint16_t)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{   1,   1, false, false },
			{   2,   0, false,  true }, // drop.
			{   3,   0, false,  true }, // drop.
			{   4,   0, false,  true }, // drop.
			{   5,   0, false,  true }, // drop.
			{   6,   0, false,  true }, // drop.
			{   7,   0, false,  true }, // drop.
			{   8,   0, false,  true }, // drop.
			{   9,   0, false,  true }, // drop.
			{ 120, 112, false, false },
			{ 121, 113, false, false },
			{ 122, 114, false, false },
			{ 123, 115, false, false },
			{ 124, 116, false, false },
			{ 125, 117, false, false },
			{ 126, 118, false, false },
			{ 127, 119, false, false },
			{ 128, 120, false, false },
			{ 129, 121, false, false },
			{ 130, 122, false, false },
			{ 131, 123, false, false },
			{ 132, 124, false, false },
			{ 133, 125, false, false },
			{ 134, 126, false, false },
			{ 135, 127, false, false },
			{ 136, 128, false, false },
			{ 137, 129, false, false },
			{ 138, 130, false, false },
			{ 139, 131, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("drop many inputs at the beginning (using uint8_t)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint8_t>> inputs =
		{
			{   1,   1, false, false },
			{   2,   0, false,  true }, // drop.
			{   3,   0, false,  true }, // drop.
			{   4,   0, false,  true }, // drop.
			{   5,   0, false,  true }, // drop.
			{   6,   0, false,  true }, // drop.
			{   7,   0, false,  true }, // drop.
			{   8,   0, false,  true }, // drop.
			{   9,   0, false,  true }, // drop.
			{ 120, 112, false, false },
			{ 121, 113, false, false },
			{ 122, 114, false, false },
			{ 123, 115, false, false },
			{ 124, 116, false, false },
			{ 125, 117, false, false },
			{ 126, 118, false, false },
			{ 127, 119, false, false },
			{ 128, 120, false, false },
			{ 129, 121, false, false },
			{ 130, 122, false, false },
			{ 131, 123, false, false },
			{ 132, 124, false, false },
			{ 133, 125, false, false },
			{ 134, 126, false, false },
			{ 135, 127, false, false },
			{ 136, 128, false, false },
			{ 137, 129, false, false },
			{ 138, 130, false, false },
			{ 139, 131, false, false }
		};
		// clang-format on

		SeqManager<uint8_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("drop many inputs at the beginning (using uint16_t with high values)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{     1,     1, false, false },
			{     2,     0, false,  true }, // drop.
			{     3,     0, false,  true }, // drop.
			{     4,     0, false,  true }, // drop.
			{     5,     0, false,  true }, // drop.
			{     6,     0, false,  true }, // drop.
			{     7,     0, false,  true }, // drop.
			{     8,     0, false,  true }, // drop.
			{     9,     0, false,  true }, // drop.
			{ 32768, 32760, false, false },
			{ 32769, 32761, false, false },
			{ 32770, 32762, false, false },
			{ 32771, 32763, false, false },
			{ 32772, 32764, false, false },
			{ 32773, 32765, false, false },
			{ 32774, 32766, false, false },
			{ 32775, 32767, false, false },
			{ 32776, 32768, false, false },
			{ 32777, 32769, false, false },
			{ 32778, 32770, false, false },
			{ 32779, 32771, false, false },
			{ 32780, 32772, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}

	SECTION("sync and drop some input near max-value")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ 65530,  1,  true, false },
			{ 65531,  2, false, false },
			{ 65532,  3, false, false },
			{ 65533,  0, false, true  },
			{ 65534,  0, false, true  },
			{ 65535,  4, false, false },
			{     0,  5, false, false },
			{     1,  6, false, false },
			{     2,  7, false, false },
			{     3,  8, false, false }
		};
		// clang-format on

		SeqManager<uint16_t> seqManager;
		validate(seqManager, inputs);
	}
}
