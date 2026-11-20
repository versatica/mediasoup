#include "common.hpp"
#include "RTC/SeqManager.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits> // std::numeric_limits
#include <vector>

namespace
{
	template<typename T>
	struct TestSeqManagerInput
	{
		T input{ 0 };
		// No value means `Input()` is expected to return false.
		std::optional<T> output;
		bool sync{ false };
		bool drop{ false };
		int64_t maxInput{ -1 };
	};

	/**
	 * Feed every element to `seqManager` in order: `Sync(input - 1)` first when
	 * the element asks for it, then `Drop()` or `Input()`.
	 *
	 * An element with no `output` expects `Input()` to return false. An element
	 * leaving `maxInput` at -1 does not check `GetMaxInput()`.
	 *
	 * @returns The first mismatch found as an { actual, expected } pair, or
	 *   { 0, 0 } when everything matched, so that a single
	 *   `REQUIRE(actual == expected)` in the caller reports both values.
	 */
	template<typename T, uint8_t N>
	std::pair<T, T> validate(
	  RTC::SeqManager<T, N> seqManager, const std::vector<TestSeqManagerInput<T>>& inputs)
	{
		for (const auto& element : inputs)
		{
			if (element.sync)
			{
				seqManager.Sync(element.input - 1);
			}

			if (element.drop)
			{
				seqManager.Drop(element.input);
			}
			else
			{
				T output{ 0 };
				const bool forwarded = seqManager.Input(element.input, output);

				if (forwarded != element.output.has_value())
				{
					return std::make_pair(static_cast<T>(forwarded), static_cast<T>(element.output.has_value()));
				}

				// `Input()` only writes `output` when it returns true, so there is nothing
				// to compare otherwise.
				if (forwarded && output != element.output.value())
				{
					return std::make_pair(output, element.output.value());
				}
			}

			// Checked for dropped inputs too, `Drop()` also moves `maxInput` forward.
			if (element.maxInput != -1)
			{
				if (element.maxInput != seqManager.GetMaxInput())
				{
					return std::make_pair(seqManager.GetMaxInput(), static_cast<T>(element.maxInput));
				}
			}
		}

		return std::make_pair(0, 0);
	}
} // namespace

SCENARIO("SeqManager", "[seqmanager]")
{
	constexpr uint16_t MaxNumberFor15Bits = (1 << 15) - 1;

	SECTION("0 is greater than 65000")
	{
		REQUIRE(RTC::SeqManager<uint16_t>::IsSeqHigherThan(0, 65000) == true);
	}

	SECTION("0 is greater than 32500 in range 15")
	{
		REQUIRE(RTC::SeqManager<uint16_t, 15>::IsSeqHigherThan(0, 32500) == true);
	}

	SECTION("receive ordered numbers, no sync, no drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output =  0 },
			{ .input =  1, .output =  1 },
			{ .input =  2, .output =  2 },
			{ .input =  3, .output =  3 },
			{ .input =  4, .output =  4 },
			{ .input =  5, .output =  5 },
			{ .input =  6, .output =  6 },
			{ .input =  7, .output =  7 },
			{ .input =  8, .output =  8 },
			{ .input =  9, .output =  9 },
			{ .input = 10, .output = 10 },
			{ .input = 11, .output = 11 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive ordered numbers, sync, no drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output = 0               },
			{ .input =  1, .output = 1               },
			{ .input =  2, .output = 2               },
			{ .input = 80, .output = 3, .sync = true },
			{ .input = 81, .output = 4               },
			{ .input = 82, .output = 5               },
			{ .input = 83, .output = 6               },
			{ .input = 84, .output = 7               }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive ordered numbers, sync, drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output =    0               },
			{ .input =  1, .output =    1               },
			{ .input =  2, .output =    2               },
			{ .input =  3, .output =    3               },
			{ .input =  4, .output =    4, .sync = true },
			{ .input =  5, .output =    5               },
			{ .input =  6, .output =    6               },
			{ .input =  7, .output =    7, .sync = true },
			{ .input =  8, .drop   = true               },
			{ .input =  9, .output =    8               },
			{ .input = 11, .drop   = true               },
			{ .input = 10, .output =    9               },
			{ .input = 12, .output =   10               }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive ordered wrapped numbers")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 65533, .output = 65533 },
			{ .input = 65534, .output = 65534 },
			{ .input = 65535, .output = 65535 },
			{ .input =     0, .output =     0 },
			{ .input =     1, .output =     1 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive sequence numbers with a big jump")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =    0, .output =    0 },
			{ .input =    1, .output =    1 },
			{ .input = 1000, .output = 1000 },
			{ .input = 1001, .output = 1001 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive out of order numbers with a big jump")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     4, .output =     4 },
			{ .input =     3, .output =     3 },
			{ .input = 65535, .output = 65535 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive mixed numbers with a big jump, drop before jump")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =   0, .output =    0 },
			{ .input =   1, .drop   = true },
			{ .input = 100, .output =   99 },
			{ .input = 100, .output =   99 },
			{ .input = 103, .drop   = true },
			{ .input = 101, .output =  100 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive mixed numbers with a big jump, drop after jump")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =   0, .output =    0 },
			{ .input =   1, .output =    1 },
			{ .input = 100, .drop   = true },
			{ .input = 103, .drop   = true },
			{ .input = 101, .output =  100 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("drop, receive numbers newer and older than the one dropped")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output =    0 },
			{ .input = 2, .drop   = true },
			{ .input = 3, .output =    2 },
			{ .input = 4, .output =    3 },
			{ .input = 1, .output =    1 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive mixed numbers, sync, drop")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     0, .output =    0               },
			{ .input =     1, .output =    1               },
			{ .input =     2, .output =    2               },
			{ .input =     3, .output =    3               },
			{ .input =     7, .output =    7               },
			{ .input =     6, .drop   = true               },
			{ .input =     8, .output =    8               },
			{ .input =    10, .output =   10               },
			{ .input =     9, .output =    9               },
			{ .input =    11, .output =   11               },
			{ .input =     0, .output =   12, .sync = true },
			{ .input =     2, .output =   14               },
			{ .input =     3, .output =   15               },
			{ .input =     4, .output =   16               },
			{ .input =     5, .output =   17               },
			{ .input =     6, .output =   18               },
			{ .input =     7, .output =   19               },
			{ .input =     8, .output =   20               },
			{ .input =     9, .output =   21               },
			{ .input =    10, .output =   22               },
			{ .input =     9, .drop   = true               },
			{ .input =    61, .output =   23, .sync = true },
			{ .input =    62, .output =   24               },
			{ .input =    63, .output =   25               },
			{ .input =    64, .output =   26               },
			{ .input =    65, .output =   27               },
			{ .input =    11, .output =   28, .sync = true },
			{ .input =    12, .output =   29               },
			{ .input =    13, .output =   30               },
			{ .input =    14, .output =   31               },
			{ .input =    15, .output =   32               },
			{ .input =     1, .output =   33, .sync = true },
			{ .input =     2, .output =   34               },
			{ .input =     3, .output =   35               },
			{ .input =     4, .output =   36               },
			{ .input =     5, .output =   37               },
			{ .input = 65533, .output =   38, .sync = true },
			{ .input = 65534, .output =   39               },
			{ .input = 65535, .output =   40               },
			{ .input =     0, .output =   41, .sync = true },
			{ .input =     1, .output =   42               },
			{ .input =     3, .drop   = true               },
			{ .input =     4, .output =   44               },
			{ .input =     5, .output =   45               },
			{ .input =     6, .output =   46               },
			{ .input =     7, .output =   47               }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive ordered numbers, sync, no drop, increase input")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output = 0               },
			{ .input =  1, .output = 1               },
			{ .input =  2, .output = 2               },
			{ .input = 80, .output = 3, .sync = true },
			{ .input = 81, .output = 4               },
			{ .input = 82, .output = 5               },
			{ .input = 83, .output = 6               },
			{ .input = 84, .output = 7               }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("drop many inputs at the beginning (using uint16_t)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =   1, .output =    1 },
			{ .input =   2, .drop   = true },
			{ .input =   3, .drop   = true },
			{ .input =   4, .drop   = true },
			{ .input =   5, .drop   = true },
			{ .input =   6, .drop   = true },
			{ .input =   7, .drop   = true },
			{ .input =   8, .drop   = true },
			{ .input =   9, .drop   = true },
			{ .input = 120, .output =  112 },
			{ .input = 121, .output =  113 },
			{ .input = 122, .output =  114 },
			{ .input = 123, .output =  115 },
			{ .input = 124, .output =  116 },
			{ .input = 125, .output =  117 },
			{ .input = 126, .output =  118 },
			{ .input = 127, .output =  119 },
			{ .input = 128, .output =  120 },
			{ .input = 129, .output =  121 },
			{ .input = 130, .output =  122 },
			{ .input = 131, .output =  123 },
			{ .input = 132, .output =  124 },
			{ .input = 133, .output =  125 },
			{ .input = 134, .output =  126 },
			{ .input = 135, .output =  127 },
			{ .input = 136, .output =  128 },
			{ .input = 137, .output =  129 },
			{ .input = 138, .output =  130 },
			{ .input = 139, .output =  131 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("drop many inputs at the beginning (using uint8_t)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint8_t>> inputs =
		{
			{ .input =   1, .output =    1 },
			{ .input =   2, .drop   = true },
			{ .input =   3, .drop   = true },
			{ .input =   4, .drop   = true },
			{ .input =   5, .drop   = true },
			{ .input =   6, .drop   = true },
			{ .input =   7, .drop   = true },
			{ .input =   8, .drop   = true },
			{ .input =   9, .drop   = true },
			{ .input = 120, .output =  112 },
			{ .input = 121, .output =  113 },
			{ .input = 122, .output =  114 },
			{ .input = 123, .output =  115 },
			{ .input = 124, .output =  116 },
			{ .input = 125, .output =  117 },
			{ .input = 126, .output =  118 },
			{ .input = 127, .output =  119 },
			{ .input = 128, .output =  120 },
			{ .input = 129, .output =  121 },
			{ .input = 130, .output =  122 },
			{ .input = 131, .output =  123 },
			{ .input = 132, .output =  124 },
			{ .input = 133, .output =  125 },
			{ .input = 134, .output =  126 },
			{ .input = 135, .output =  127 },
			{ .input = 136, .output =  128 },
			{ .input = 137, .output =  129 },
			{ .input = 138, .output =  130 },
			{ .input = 139, .output =  131 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint8_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive mixed numbers, sync, drop in range 15")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     0, .output =    0               },
			{ .input =     1, .output =    1               },
			{ .input =     2, .output =    2               },
			{ .input =     3, .output =    3               },
			{ .input =     7, .output =    7               },
			{ .input =     6, .drop   = true               },
			{ .input =     8, .output =    8               },
			{ .input =    10, .output =   10               },
			{ .input =     9, .output =    9               },
			{ .input =    11, .output =   11               },
			{ .input =     0, .output =   12, .sync = true },
			{ .input =     2, .output =   14               },
			{ .input =     3, .output =   15               },
			{ .input =     4, .output =   16               },
			{ .input =     5, .output =   17               },
			{ .input =     6, .output =   18               },
			{ .input =     7, .output =   19               },
			{ .input =     8, .output =   20               },
			{ .input =     9, .output =   21               },
			{ .input =    10, .output =   22               },
			{ .input =     9, .drop   = true               },
			{ .input =    61, .output =   23, .sync = true },
			{ .input =    62, .output =   24               },
			{ .input =    63, .output =   25               },
			{ .input =    64, .output =   26               },
			{ .input =    65, .output =   27               },
			{ .input =    11, .output =   28, .sync = true },
			{ .input =    12, .output =   29               },
			{ .input =    13, .output =   30               },
			{ .input =    14, .output =   31               },
			{ .input =    15, .output =   32               },
			{ .input =     1, .output =   33, .sync = true },
			{ .input =     2, .output =   34               },
			{ .input =     3, .output =   35               },
			{ .input =     4, .output =   36               },
			{ .input =     5, .output =   37               },
			{ .input = 32767, .output =   38, .sync = true },
			{ .input = 32768, .output =   39               },
			{ .input = 32769, .output =   40               },
			{ .input =     0, .output =   41, .sync = true },
			{ .input =     1, .output =   42               },
			{ .input =     3, .drop   = true               },
			{ .input =     4, .output =   44               },
			{ .input =     5, .output =   45               },
			{ .input =     6, .output =   46               },
			{ .input =     7, .output =   47               }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("drop many inputs at the beginning (using uint16_t with high values)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     1, .output =     1 },
			{ .input =     2, .drop   =  true },
			{ .input =     3, .drop   =  true },
			{ .input =     4, .drop   =  true },
			{ .input =     5, .drop   =  true },
			{ .input =     6, .drop   =  true },
			{ .input =     7, .drop   =  true },
			{ .input =     8, .drop   =  true },
			{ .input =     9, .drop   =  true },
			{ .input = 32768, .output = 32760 },
			{ .input = 32769, .output = 32761 },
			{ .input = 32770, .output = 32762 },
			{ .input = 32771, .output = 32763 },
			{ .input = 32772, .output = 32764 },
			{ .input = 32773, .output = 32765 },
			{ .input = 32774, .output = 32766 },
			{ .input = 32775, .output = 32767 },
			{ .input = 32776, .output = 32768 },
			{ .input = 32777, .output = 32769 },
			{ .input = 32778, .output = 32770 },
			{ .input = 32779, .output = 32771 },
			{ .input = 32780, .output = 32772 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("sync and drop some input near max-value")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 65530, .output =    1, .sync = true },
			{ .input = 65531, .output =    2               },
			{ .input = 65532, .output =    3               },
			{ .input = 65533, .drop   = true               },
			{ .input = 65534, .drop   = true               },
			{ .input = 65535, .output =    4               },
			{ .input =     0, .output =    5               },
			{ .input =     1, .output =    6               },
			{ .input =     2, .output =    7               },
			{ .input =     3, .output =    8               }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("drop many inputs at the beginning (using uint16_t range 15 with high values)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     1, .output =     1 },
			{ .input =     2, .drop   =  true },
			{ .input =     3, .drop   =  true },
			{ .input =     4, .drop   =  true },
			{ .input =     5, .drop   =  true },
			{ .input =     6, .drop   =  true },
			{ .input =     7, .drop   =  true },
			{ .input =     8, .drop   =  true },
			{ .input =     9, .drop   =  true },
			{ .input = 16384, .output = 16376 },
			{ .input = 16385, .output = 16377 },
			{ .input = 16386, .output = 16378 },
			{ .input = 16387, .output = 16379 },
			{ .input = 16388, .output = 16380 },
			{ .input = 16389, .output = 16381 },
			{ .input = 16390, .output = 16382 },
			{ .input = 16391, .output = 16383 },
			{ .input = 16392, .output = 16384 },
			{ .input = 16393, .output = 16385 },
			{ .input = 16394, .output = 16386 },
			{ .input = 16395, .output = 16387 },
			{ .input = 16396, .output = 16388 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("sync and drop some input near max-value in a 15bit range")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 32762, .output =    1, .sync     =  true, .maxInput = 32762 },
			{ .input = 32763, .output =    2, .maxInput = 32763                    },
			{ .input = 32764, .output =    3, .maxInput = 32764                    },
			{ .input = 32765, .drop   = true, .maxInput = 32765                    },
			{ .input = 32766, .drop   = true, .maxInput = 32766                    },
			{ .input = 32767, .output =    4, .maxInput = 32767                    },
			{ .input =     0, .output =    5, .maxInput =     0                    },
			{ .input =     1, .output =    6, .maxInput =     1                    },
			{ .input =     2, .output =    7, .maxInput =     2                    },
			{ .input =     3, .output =    8, .maxInput =     3                    }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("should update all values during multiple roll overs")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output = 1, .sync = true, .maxInput = 0 }
		};
		for (uint16_t j = 0; j < 3; ++j) {
			for (uint16_t i = 1; i < std::numeric_limits<uint16_t>::max(); ++i) {
				const uint16_t output = i + 1;
				inputs.push_back({ .input = i, .output = output, .maxInput = i });
			}
		}
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("should update all values during multiple roll overs (15 bits range)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output = 1, .sync = true, .maxInput = 0 }
		};
		for (uint16_t j = 0; j < 3; ++j) {
			for (uint16_t i = 1; i < MaxNumberFor15Bits; ++i) {
				const uint16_t output = i + 1;
				inputs.push_back({ .input = i, .output = output, .maxInput = i });
			}
		}
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("should produce same output for same old input before drop (15 bits range)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 10, .output =    1, .sync = true },
			{ .input = 11, .output =    2               },
			{ .input = 12, .output =    3               },
			{ .input = 13, .output =    4               },
			{ .input = 14, .drop   = true               },
			{ .input = 15, .output =    5               },
			{ .input = 12, .output =    3               }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t, 15>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("should properly clean previous cycle drops")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint8_t>> inputs =
		{
			{ .input = 1, .output =    1 },
			{ .input = 2, .drop   = true },
			{ .input = 3, .output =    2 },
			{ .input = 4, .output =    3 },
			{ .input = 5, .output =    4 },
			{ .input = 6, .output =    5 },
			{ .input = 7, .output =    6 },
			{ .input = 0, .output =    7 },
			{ .input = 1, .output =    0 },
			{ .input = 2, .output =    1 },
			{ .input = 3, .output =    2 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint8_t, 3>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("dropped inputs to be removed going out of range, 1.")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 36964, .output =        36964 },
			{ .input = 25923, .drop   =         true },
			{ .input = 25701, .output =        25701 },
			{ .input = 17170, .drop   =         true },
			{ .input = 25923, .output =        25923 },
			{ .input =  4728, .drop   =         true },
			{ .input = 17170, .output =        17170 },
			{ .input = 30738, .drop   =         true },
			{ .input =  4728, .output =         4728 },
			{ .input =  4806, .drop   =         true },
			{ .input = 30738, .output =        30738 },
			{ .input = 50886, .drop   =         true },
			{ .input =  4806, .output =         4805 },
			{ .input = 50774, .drop   =         true },
			{ .input = 50886, .output = std::nullopt }, // Previously dropped.
			{ .input = 22136, .drop   =         true },
			{ .input = 50774, .output =        50772 },
			{ .input = 30910, .drop   =         true },
			{ .input = 22136, .output = std::nullopt }, // Previously dropped.
			{ .input = 48862, .drop   =         true },
			{ .input = 30910, .output =        30908 },
			{ .input = 56832, .drop   =         true },
			{ .input = 48862, .output =        48860 },
			{ .input =     2, .drop   =         true },
			{ .input = 56832, .output = std::nullopt }, // Previously dropped.
			{ .input =   530, .drop   =         true },
			{ .input =     2, .output = std::nullopt } // Previously dropped.
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("dropped inputs to be removed go out of range, 2.")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 36960, .output =        36960 },
			{ .input =  3328, .drop   =         true },
			{ .input = 24589, .output =        24588 },
			{ .input =   120, .drop   =         true },
			{ .input =  3328, .output = std::nullopt }, // Previously dropped.
			{ .input = 30848, .drop   =         true },
			{ .input =   120, .output =          120 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("dropped inputs to be removed go out of range, 3.")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 36964, .output = 36964 },
			{ .input = 65396, .drop   =  true },
			{ .input = 25855, .output = 25854 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	// `ClearDropped()` stops at the first entry that is not from the previous
	// cycle. A newer drop can sort before such entries, leaving them behind
	// forever, which is why `Drop()` must clear before inserting.
	//
	// The stream below is strictly increasing once unwrapped:
	//
	//   190, 200, 296, 342, 454
	//
	// 200 and 342 are dropped, so 454 must be shifted down by 2, giving
	// 454 - 2 = 452, that is 196 once wrapped.
	SECTION("a newer drop does not prevent clearing a previous cycle drop (using uint8_t)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint8_t>> inputs =
		{
			{ .input = 190, .output =  190 },
			{ .input = 200, .drop   = true },
			{ .input =  40, .output =   39 }, // 296 unwrapped, one drop below it.
			{ .input =  86, .drop   = true }, // 200 belongs to the previous cycle now.
			{ .input = 198, .output =  196 } // 454 unwrapped, two drops below it.
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint8_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	// Same scenario using RTP sequence numbers. Unwrapped the stream is:
	//
	//   48640, 51200, 75776, 87552, 116224
	//
	// 51200 and 87552 are dropped, so 116224 must be shifted down by 2, giving
	// 116222, that is 50686 once wrapped.
	SECTION("a newer drop does not prevent clearing a previous cycle drop (using uint16_t)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 48640, .output = 48640 },
			{ .input = 51200, .drop   =  true },
			{ .input = 10240, .output = 10239 }, // 75776 unwrapped, one drop below it.
			{ .input = 22016, .drop   =  true }, // 51200 belongs to the previous cycle now.
			{ .input = 50688, .output = 50686 } // 116224 unwrapped, two drops below it.
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive ordered numbers, no sync, no drop (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output = 1000 },
			{ .input =  1, .output = 1001 },
			{ .input =  2, .output = 1002 },
			{ .input =  3, .output = 1003 },
			{ .input =  4, .output = 1004 },
			{ .input =  5, .output = 1005 },
			{ .input =  6, .output = 1006 },
			{ .input =  7, .output = 1007 },
			{ .input =  8, .output = 1008 },
			{ .input =  9, .output = 1009 },
			{ .input = 10, .output = 1010 },
			{ .input = 11, .output = 1011 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive ordered numbers, sync, no drop (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output = 2000               },
			{ .input =  1, .output = 2001               },
			{ .input =  2, .output = 2002               },
			{ .input = 80, .output = 2003, .sync = true },
			{ .input = 81, .output = 2004               },
			{ .input = 82, .output = 2005               },
			{ .input = 83, .output = 2006               },
			{ .input = 84, .output = 2007               }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 2000 }, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 2000 }, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive ordered numbers, sync, drop (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output = 3000               },
			{ .input =  1, .output = 3001               },
			{ .input =  2, .output = 3002               },
			{ .input =  3, .output = 3003               },
			{ .input =  4, .output = 3004, .sync = true },
			{ .input =  5, .output = 3005               },
			{ .input =  6, .output = 3006               },
			{ .input =  7, .output = 3007, .sync = true },
			{ .input =  8, .drop   = true               },
			{ .input =  9, .output = 3008               },
			{ .input = 11, .drop   = true               },
			{ .input = 10, .output = 3009               },
			{ .input = 12, .output = 3010               }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 3000 }, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 3000 }, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive ordered wrapped numbers (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 65533, .output =  997 },
			{ .input = 65534, .output =  998 },
			{ .input = 65535, .output =  999 },
			{ .input =     0, .output = 1000 },
			{ .input =     1, .output = 1001 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive sequence numbers with a big jump (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs1 =
		{
			{ .input =    0, .output = 32000 },
			{ .input =    1, .output = 32001 },
			{ .input = 1000, .output = 33000 },
			{ .input = 1001, .output = 33001 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 32000 }, inputs1);

		REQUIRE(actual == expected);

		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs2 =
		{
			{ .input =    0, .output = 32000 },
			{ .input =    1, .output = 32001 },
			{ .input = 1000, .output =   232 },
			{ .input = 1001, .output =   233 }
		};
		// clang-format on

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 32000 }, inputs2);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive out of order numbers with a big jump (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     4, .output = 1004 },
			{ .input =     3, .output = 1003 },
			{ .input = 65535, .output =  999 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive mixed numbers with a big jump, drop before jump (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =   0, .output = 1000 },
			{ .input =   1, .drop   = true },
			{ .input = 100, .output = 1099 },
			{ .input = 100, .output = 1099 },
			{ .input = 103, .drop   = true },
			{ .input = 101, .output = 1100 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive mixed numbers with a big jump, drop after jump (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =   0, .output = 2000 },
			{ .input =   1, .output = 2001 },
			{ .input = 100, .drop   = true },
			{ .input = 103, .drop   = true },
			{ .input = 101, .output = 2100 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 2000 }, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 2000 }, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("drop, receive numbers newer and older than the one dropped (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output = 2000 },
			{ .input = 2, .drop   = true },
			{ .input = 3, .output = 2002 },
			{ .input = 4, .output = 2003 },
			{ .input = 1, .output = 2001 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 2000 }, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 2000 }, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("receive mixed numbers, sync, drop (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     0, .output = 10000               },
			{ .input =     1, .output = 10001               },
			{ .input =     2, .output = 10002               },
			{ .input =     3, .output = 10003               },
			{ .input =     7, .output = 10007               },
			{ .input =     6, .drop   =  true               },
			{ .input =     8, .output = 10008               },
			{ .input =    10, .output = 10010               },
			{ .input =     9, .output = 10009               },
			{ .input =    11, .output = 10011               },
			{ .input =     0, .output = 10012, .sync = true },
			{ .input =     2, .output = 10014               },
			{ .input =     3, .output = 10015               },
			{ .input =     4, .output = 10016               },
			{ .input =     5, .output = 10017               },
			{ .input =     6, .output = 10018               },
			{ .input =     7, .output = 10019               },
			{ .input =     8, .output = 10020               },
			{ .input =     9, .output = 10021               },
			{ .input =    10, .output = 10022               },
			{ .input =     9, .drop   =  true               },
			{ .input =    61, .output = 10023, .sync = true },
			{ .input =    62, .output = 10024               },
			{ .input =    63, .output = 10025               },
			{ .input =    64, .output = 10026               },
			{ .input =    65, .output = 10027               },
			{ .input =    11, .output = 10028, .sync = true },
			{ .input =    12, .output = 10029               },
			{ .input =    13, .output = 10030               },
			{ .input =    14, .output = 10031               },
			{ .input =    15, .output = 10032               },
			{ .input =     1, .output = 10033, .sync = true },
			{ .input =     2, .output = 10034               },
			{ .input =     3, .output = 10035               },
			{ .input =     4, .output = 10036               },
			{ .input =     5, .output = 10037               },
			{ .input = 65533, .output = 10038, .sync = true },
			{ .input = 65534, .output = 10039               },
			{ .input = 65535, .output = 10040               },
			{ .input =     0, .output = 10041, .sync = true },
			{ .input =     1, .output = 10042               },
			{ .input =     3, .drop   =  true               },
			{ .input =     4, .output = 10044               },
			{ .input =     5, .output = 10045               },
			{ .input =     6, .output = 10046               },
			{ .input =     7, .output = 10047               }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 10000 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive ordered numbers, sync, no drop, increase input (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output = 1               },
			{ .input =  1, .output = 2               },
			{ .input =  2, .output = 3               },
			{ .input = 80, .output = 4, .sync = true },
			{ .input = 81, .output = 5               },
			{ .input = 82, .output = 6               },
			{ .input = 83, .output = 7               },
			{ .input = 84, .output = 8               }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 1 }, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 1 }, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("drop many inputs at the beginning (using uint16_t) (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =   1, .output = 1001 },
			{ .input =   2, .drop   = true },
			{ .input =   3, .drop   = true },
			{ .input =   4, .drop   = true },
			{ .input =   5, .drop   = true },
			{ .input =   6, .drop   = true },
			{ .input =   7, .drop   = true },
			{ .input =   8, .drop   = true },
			{ .input =   9, .drop   = true },
			{ .input = 120, .output = 1112 },
			{ .input = 121, .output = 1113 },
			{ .input = 122, .output = 1114 },
			{ .input = 123, .output = 1115 },
			{ .input = 124, .output = 1116 },
			{ .input = 125, .output = 1117 },
			{ .input = 126, .output = 1118 },
			{ .input = 127, .output = 1119 },
			{ .input = 128, .output = 1120 },
			{ .input = 129, .output = 1121 },
			{ .input = 130, .output = 1122 },
			{ .input = 131, .output = 1123 },
			{ .input = 132, .output = 1124 },
			{ .input = 133, .output = 1125 },
			{ .input = 134, .output = 1126 },
			{ .input = 135, .output = 1127 },
			{ .input = 136, .output = 1128 },
			{ .input = 137, .output = 1129 },
			{ .input = 138, .output = 1130 },
			{ .input = 139, .output = 1131 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual == expected);

		const auto [actual15Bits, expected15Bits] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual15Bits == expected15Bits);
	}

	SECTION("drop many inputs at the beginning (using uint8_t) (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint8_t>> inputs =
		{
			{ .input =   1, .output =  201 },
			{ .input =   2, .drop   = true },
			{ .input =   3, .drop   = true },
			{ .input =   4, .drop   = true },
			{ .input =   5, .drop   = true },
			{ .input =   6, .drop   = true },
			{ .input =   7, .drop   = true },
			{ .input =   8, .drop   = true },
			{ .input =   9, .drop   = true },
			{ .input = 120, .output =   56 },
			{ .input = 121, .output =   57 },
			{ .input = 122, .output =   58 },
			{ .input = 123, .output =   59 },
			{ .input = 124, .output =   60 },
			{ .input = 125, .output =   61 },
			{ .input = 126, .output =   62 },
			{ .input = 127, .output =   63 },
			{ .input = 128, .output =   64 },
			{ .input = 129, .output =   65 },
			{ .input = 130, .output =   66 },
			{ .input = 131, .output =   67 },
			{ .input = 132, .output =   68 },
			{ .input = 133, .output =   69 },
			{ .input = 134, .output =   70 },
			{ .input = 135, .output =   71 },
			{ .input = 136, .output =   72 },
			{ .input = 137, .output =   73 },
			{ .input = 138, .output =   74 },
			{ .input = 139, .output =   75 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint8_t>{ /*initialOutput*/ 200 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive mixed numbers, sync, drop in range 15 (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     0, .output =  100               },
			{ .input =     1, .output =  101               },
			{ .input =     2, .output =  102               },
			{ .input =     3, .output =  103               },
			{ .input =     7, .output =  107               },
			{ .input =     6, .drop   = true               },
			{ .input =     8, .output =  108               },
			{ .input =    10, .output =  110               },
			{ .input =     9, .output =  109               },
			{ .input =    11, .output =  111               },
			{ .input =     0, .output =  112, .sync = true },
			{ .input =     2, .output =  114               },
			{ .input =     3, .output =  115               },
			{ .input =     4, .output =  116               },
			{ .input =     5, .output =  117               },
			{ .input =     6, .output =  118               },
			{ .input =     7, .output =  119               },
			{ .input =     8, .output =  120               },
			{ .input =     9, .output =  121               },
			{ .input =    10, .output =  122               },
			{ .input =     9, .drop   = true               },
			{ .input =    61, .output =  123, .sync = true },
			{ .input =    62, .output =  124               },
			{ .input =    63, .output =  125               },
			{ .input =    64, .output =  126               },
			{ .input =    65, .output =  127               },
			{ .input =    11, .output =  128, .sync = true },
			{ .input =    12, .output =  129               },
			{ .input =    13, .output =  130               },
			{ .input =    14, .output =  131               },
			{ .input =    15, .output =  132               },
			{ .input =     1, .output =  133, .sync = true },
			{ .input =     2, .output =  134               },
			{ .input =     3, .output =  135               },
			{ .input =     4, .output =  136               },
			{ .input =     5, .output =  137               },
			{ .input = 32767, .output =  138, .sync = true },
			{ .input = 32768, .output =  139               },
			{ .input = 32769, .output =  140               },
			{ .input =     0, .output =  141, .sync = true },
			{ .input =     1, .output =  142               },
			{ .input =     3, .drop   = true               },
			{ .input =     4, .output =  144               },
			{ .input =     5, .output =  145               },
			{ .input =     6, .output =  146               },
			{ .input =     7, .output =  147               }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 100 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("drop many inputs at the beginning (using uint16_t with high values) (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     1, .output =   201 },
			{ .input =     2, .drop   =  true },
			{ .input =     3, .drop   =  true },
			{ .input =     4, .drop   =  true },
			{ .input =     5, .drop   =  true },
			{ .input =     6, .drop   =  true },
			{ .input =     7, .drop   =  true },
			{ .input =     8, .drop   =  true },
			{ .input =     9, .drop   =  true },
			{ .input = 32768, .output = 32960 },
			{ .input = 32769, .output = 32961 },
			{ .input = 32770, .output = 32962 },
			{ .input = 32771, .output = 32963 },
			{ .input = 32772, .output = 32964 },
			{ .input = 32773, .output = 32965 },
			{ .input = 32774, .output = 32966 },
			{ .input = 32775, .output = 32967 },
			{ .input = 32776, .output = 32968 },
			{ .input = 32777, .output = 32969 },
			{ .input = 32778, .output = 32970 },
			{ .input = 32779, .output = 32971 },
			{ .input = 32780, .output = 32972 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 200 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("sync and drop some input near max-value (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 65530, .output =  201, .sync = true },
			{ .input = 65531, .output =  202               },
			{ .input = 65532, .output =  203               },
			{ .input = 65533, .drop   = true               },
			{ .input = 65534, .drop   = true               },
			{ .input = 65535, .output =  204               },
			{ .input =     0, .output =  205               },
			{ .input =     1, .output =  206               },
			{ .input =     2, .output =  207               },
			{ .input =     3, .output =  208               }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 200 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION(
	  "drop many inputs at the beginning (using uint16_t range 15 with high values) (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =     1, .output =   101 },
			{ .input =     2, .drop   =  true },
			{ .input =     3, .drop   =  true },
			{ .input =     4, .drop   =  true },
			{ .input =     5, .drop   =  true },
			{ .input =     6, .drop   =  true },
			{ .input =     7, .drop   =  true },
			{ .input =     8, .drop   =  true },
			{ .input =     9, .drop   =  true },
			{ .input = 16384, .output = 16476 },
			{ .input = 16385, .output = 16477 },
			{ .input = 16386, .output = 16478 },
			{ .input = 16387, .output = 16479 },
			{ .input = 16388, .output = 16480 },
			{ .input = 16389, .output = 16481 },
			{ .input = 16390, .output = 16482 },
			{ .input = 16391, .output = 16483 },
			{ .input = 16392, .output = 16484 },
			{ .input = 16393, .output = 16485 },
			{ .input = 16394, .output = 16486 },
			{ .input = 16395, .output = 16487 },
			{ .input = 16396, .output = 16488 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 100 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("sync and drop some input near max-value in a 15bit range (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 32762, .output =  101, .sync     =  true, .maxInput = 32762 },
			{ .input = 32763, .output =  102, .maxInput = 32763                    },
			{ .input = 32764, .output =  103, .maxInput = 32764                    },
			{ .input = 32765, .drop   = true, .maxInput = 32765                    },
			{ .input = 32766, .drop   = true, .maxInput = 32766                    },
			{ .input = 32767, .output =  104, .maxInput = 32767                    },
			{ .input =     0, .output =  105, .maxInput =     0                    },
			{ .input =     1, .output =  106, .maxInput =     1                    },
			{ .input =     2, .output =  107, .maxInput =     2                    },
			{ .input =     3, .output =  108, .maxInput =     3                    }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 100 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("should update all values during multiple roll overs (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output = 101, .sync = true, .maxInput = 0 }
		};
		// clang-format on

		for (uint16_t j = 0; j < 3; ++j)
		{
			for (uint16_t i = 1; i < std::numeric_limits<uint16_t>::max(); ++i)
			{
				const uint16_t output = (i + 1 + 100) & std::numeric_limits<uint16_t>::max();
				inputs.push_back({ .input = i, .output = output, .maxInput = i });
			}
		}

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 100 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("should update all values during multiple roll overs (15 bits range) (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output = 101, .sync = true, .maxInput = 0 }
		};
		// clang-format on

		for (uint16_t j = 0; j < 3; ++j)
		{
			for (uint16_t i = 1; i < MaxNumberFor15Bits; ++i)
			{
				const uint16_t output = (i + 1 + 100) & MaxNumberFor15Bits;

				inputs.push_back({ .input = i, .output = output, .maxInput = i });
			}
		}

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 100 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION(
	  "should produce same output for same old input before drop (15 bits range) (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 10, .output = 10001, .sync = true },
			{ .input = 11, .output = 10002               },
			{ .input = 12, .output = 10003               },
			{ .input = 13, .output = 10004               },
			{ .input = 14, .drop   =  true               },
			{ .input = 15, .output = 10005               },
			{ .input = 12, .output = 10003               }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t, 15>{ /*initialOutput*/ 10000 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("should properly clean previous cycle drops (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint8_t>> inputs =
		{
			{ .input = 1, .output =    3 },
			{ .input = 2, .drop   = true },
			{ .input = 3, .output =    4 },
			{ .input = 4, .output =    5 },
			{ .input = 5, .output =    6 },
			{ .input = 6, .output =    7 },
			{ .input = 7, .output =    0 },
			{ .input = 0, .output =    1 },
			{ .input = 1, .output =    2 },
			{ .input = 2, .output =    3 },
			{ .input = 3, .output =    4 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint8_t, 3>{ /*initialOutput*/ 2 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("dropped inputs to be removed going out of range, 1. (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 36964, .output =        46964 },
			{ .input = 25923, .drop   =         true },
			{ .input = 25701, .output =        35701 },
			{ .input = 17170, .drop   =         true },
			{ .input = 25923, .output =        35923 },
			{ .input =  4728, .drop   =         true },
			{ .input = 17170, .output =        27170 },
			{ .input = 30738, .drop   =         true },
			{ .input =  4728, .output =        14728 },
			{ .input =  4806, .drop   =         true },
			{ .input = 30738, .output =        40738 },
			{ .input = 50886, .drop   =         true },
			{ .input =  4806, .output =        14805 },
			{ .input = 50774, .drop   =         true },
			{ .input = 50886, .output = std::nullopt }, // Previously dropped.
			{ .input = 22136, .drop   =         true },
			{ .input = 50774, .output =        60772 },
			{ .input = 30910, .drop   =         true },
			{ .input = 22136, .output = std::nullopt }, // Previously dropped.
			{ .input = 48862, .drop   =         true },
			{ .input = 30910, .output =        40908 },
			{ .input = 56832, .drop   =         true },
			{ .input = 48862, .output =        58860 },
			{ .input =     2, .drop   =         true },
			{ .input = 56832, .output = std::nullopt }, // Previously dropped.
			{ .input =   530, .drop   =         true },
			{ .input =     2, .output = std::nullopt } // Previously dropped.
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 10000 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("dropped inputs to be removed go out of range, 2. (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 36960, .output =        37060 },
			{ .input =  3328, .drop   =         true },
			{ .input = 24589, .output =        24688 },
			{ .input =   120, .drop   =         true },
			{ .input =  3328, .output = std::nullopt }, // Previously dropped.
			{ .input = 30848, .drop   =         true },
			{ .input =   120, .output =          220 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 100 }, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("dropped inputs to be removed go out of range, 3. (with initial output)")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 36964, .output = 37964 },
			{ .input = 65396, .drop   =  true },
			{ .input = 25855, .output = 26854 }
		};
		// clang-format on

		const auto [actual, expected] =
		  validate(RTC::SeqManager<uint16_t>{ /*initialOutput*/ 1000 }, inputs);

		REQUIRE(actual == expected);
	}

	// https://github.com/versatica/mediasoup/issues/1615
	SECTION("receive dropped inputs out of order")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output =    0 },
			{ .input = 2, .drop   = true },
			{ .input = 1, .drop   = true },
			{ .input = 3, .output =    1 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive dropped inputs out of order, 2")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  0, .output =    0 },
			{ .input =  1, .output =    1 },
			{ .input =  2, .output =    2 },
			{ .input =  4, .output =    4 },
			{ .input =  5, .output =    5 },
			{ .input =  6, .drop   = true },
			{ .input =  7, .drop   = true },
			{ .input =  8, .drop   = true },
			{ .input =  3, .drop   = true },
			{ .input =  9, .drop   = true },
			{ .input = 10, .output =    6 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive dropped inputs out of order, 3")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 1, .output =    1 },
			{ .input = 2, .output =    2 },
			{ .input = 3, .output =    3 },
			{ .input = 4, .output =    4 },
			{ .input = 5, .drop   = true },
			{ .input = 6, .drop   = true },
			{ .input = 7, .drop   = true },
			{ .input = 8, .drop   = true },
			{ .input = 0, .drop   = true },
			{ .input = 9, .output =    5 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("receive dropped inputs out of order, 4")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input =  1, .output =    1 },
			{ .input =  2, .output =    2 },
			{ .input =  3, .output =    3 },
			{ .input =  4, .output =    4 },
			{ .input =  5, .output =    5 },
			{ .input =  6, .output =    6 },
			{ .input =  7, .output =    7 },
			{ .input =  8, .output =    8 },
			{ .input =  9, .output =    9 },
			{ .input = 10, .output =   10 },
			{ .input = 11, .output =   11 },
			{ .input = 12, .output =   12 },
			{ .input = 13, .output =   13 },
			{ .input = 14, .output =   14 },
			{ .input = 15, .output =   15 },
			{ .input = 16, .output =   16 },
			{ .input = 17, .output =   17 },
			{ .input = 18, .output =   18 },
			{ .input = 19, .output =   19 },
			{ .input = 21, .output =   21 },
			{ .input = 22, .output =   22 },
			{ .input = 23, .output =   23 },
			{ .input = 24, .output =   24 },
			{ .input = 25, .output =   25 },
			{ .input = 26, .output =   26 },
			{ .input = 27, .drop   = true },
			{ .input = 28, .drop   = true },
			{ .input = 29, .drop   = true },
			{ .input = 20, .output =   20 },
			{ .input = 30, .drop   = true },
			{ .input = 31, .drop   = true },
			{ .input = 32, .drop   = true },
			{ .input = 33, .drop   = true },
			{ .input = 34, .drop   = true },
			{ .input = 35, .drop   = true },
			{ .input = 36, .drop   = true },
			{ .input = 38, .drop   = true },
			{ .input = 39, .drop   = true },
			{ .input = 40, .drop   = true },
			{ .input = 41, .drop   = true },
			{ .input = 42, .drop   = true },
			{ .input = 44, .drop   = true },
			{ .input = 45, .drop   = true },
			{ .input = 46, .drop   = true },
			{ .input = 47, .output =   29 },
			{ .input = 48, .output =   30 },
			{ .input = 49, .output =   31 },
			{ .input = 51, .output =   33 },
			{ .input = 52, .output =   34 },
			{ .input = 53, .output =   35 },
			{ .input = 54, .output =   36 },
			{ .input = 55, .drop   = true },
			{ .input = 56, .drop   = true },
			{ .input = 57, .drop   = true },
			{ .input = 58, .drop   = true },
			{ .input = 59, .drop   = true },
			{ .input = 37, .drop   = true },
			{ .input = 60, .drop   = true },
			{ .input = 61, .drop   = true },
			{ .input = 62, .drop   = true },
			{ .input = 63, .drop   = true },
			{ .input = 64, .drop   = true },
			{ .input = 43, .drop   = true },
			{ .input = 50, .output =   32 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("a drop lower than the highest forwarded input is not accounted")
	{
		// clang-format off
		const std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output = 0    },
			{ .input = 1, .output = 1    },
			{ .input = 4, .drop   = true },
			{ .input = 3, .output = 3    },
			{ .input = 2, .drop   = true },
			{ .input = 5, .output = 4    },
			{ .input = 6, .output = 5    }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("many drops lower than the highest forwarded input are not accounted")
	{
		// clang-format off
		std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0,  .output = 0    },
			{ .input = 31, .drop   = true },
			{ .input = 27, .output = 27   },
			{ .input = 30, .output = 30   }
		};

		for (uint16_t input{ 1 }; input <= 26; ++input)
		{
			inputs.push_back({ .input = input, .drop = true });
		}

		inputs.push_back({ .input = 28, .drop   = true });
		inputs.push_back({ .input = 29, .drop   = true });
		inputs.push_back({ .input = 32, .output = 31   });
		inputs.push_back({ .input = 33, .output = 32   });
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("a drop of an already forwarded input is not accounted")
	{
		// clang-format off
		const std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0, .output = 0    },
			{ .input = 4, .drop   = true },
			{ .input = 3, .output = 3    },
			{ .input = 3, .drop   = true },
			{ .input = 5, .output = 4    }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("a drop higher than the highest forwarded input is accounted")
	{
		// clang-format off
		const std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 0,  .output = 0    },
			{ .input = 10, .drop   = true },
			{ .input = 5,  .output = 5    },
			{ .input = 7,  .drop   = true },
			{ .input = 6,  .output = 6    },
			{ .input = 8,  .output = 7    },
			{ .input = 9,  .output = 8    },
			{ .input = 11, .output = 9    }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	SECTION("a drop is accounted when maxForwarded lags by more than half a cycle")
	{
		// The sequence wraps at 3328, which is higher than 36960, so `maxInput`
		// moves on while `maxForwarded` stays at 36960, more than half a cycle
		// behind. 24589 is higher than `maxInput` but `IsSeqHigherThan()` cannot
		// see it above `maxForwarded`, so it is only accounted because `Drop()`
		// checks `maxInput` first. 24590 is 53166 inputs after 36960, two dropped.
		// clang-format off
		const std::vector<TestSeqManagerInput<uint16_t>> inputs =
		{
			{ .input = 36960, .output = 36960, .maxInput = 36960 },
			{ .input =  3328, .drop   =  true, .maxInput =  3328 },
			{ .input = 24589, .drop   =  true, .maxInput = 24589 },
			{ .input = 24590, .output = 24588, .maxInput = 24590 }
		};
		// clang-format on

		const auto [actual, expected] = validate(RTC::SeqManager<uint16_t>{}, inputs);

		REQUIRE(actual == expected);
	}

	// NOTE: `validate()` cannot express these: it syncs on an element and then
	// processes that very same element, so it cannot drop an input between the
	// sync and the next forwarded input.
	SECTION("a drop older than the sync point is not accounted")
	{
		RTC::SeqManager<uint16_t> seqManager;
		uint16_t output{ 0 };

		REQUIRE(seqManager.Input(0, output));
		REQUIRE(output == 0);

		// Drop an input higher than anyone seen, so the highest input seen is a
		// dropped one.
		seqManager.Drop(100);

		// Sync so the next input (101) produces the output following the last one.
		seqManager.Sync(100);

		// A drop older than the sync point must not be accounted, the output of
		// every input above it was already emitted.
		seqManager.Drop(50);

		REQUIRE(seqManager.Input(101, output));
		REQUIRE(output == 1);
	}

	SECTION("an out of order drop after a sync is accounted")
	{
		RTC::SeqManager<uint16_t> seqManager;
		uint16_t output{ 0 };

		REQUIRE(seqManager.Input(1000, output));
		REQUIRE(output == 1000);

		// Sync so the next input (1) produces the output following the last one.
		seqManager.Sync(0);

		seqManager.Drop(5);

		REQUIRE(seqManager.Input(3, output));
		REQUIRE(output == 1003);

		// Input 4 is dropped once input 3 has been forwarded, so it must be
		// accounted and input 6 must not leave a gap for it.
		seqManager.Drop(4);

		REQUIRE(seqManager.Input(6, output));
		REQUIRE(output == 1004);
	}

	SECTION("GetMaxOutput() returns the real max output (with initial output)")
	{
		RTC::SeqManager<uint16_t> seqManager(/*initialOutput*/ 1000);
		uint16_t output;

		seqManager.Input(100, output);

		// Output must be 100 + initialOutput (1000) = 1100.
		REQUIRE(output == 1100);
		// GetMaxOutput() must report the same value as the last (and highest)
		// output.
		REQUIRE(seqManager.GetMaxOutput() == 1100);

		seqManager.Input(101, output);

		REQUIRE(output == 1101);
		REQUIRE(seqManager.GetMaxOutput() == 1101);
	}
}
