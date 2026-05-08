#ifndef MS_CATCH2_MACROS_HPP
#define MS_CATCH2_MACROS_HPP

#include <catch2/catch_test_macros.hpp>
#include <string>

namespace test
{
	struct VerificationResult
	{
		bool ok;
		std::string errorMessage;
	};
} // namespace test

// clang-format off
#define _MS_REQUIRE_WITH_MESSAGE_1(verificationResult) \
	do \
	{ \
		const auto&& result = (verificationResult); \
		\
		if (!(result.ok)) \
		{ \
			FAIL(result.errorMessage); \
		} \
	} while (false)

// clang-format off
#define _MS_REQUIRE_WITH_MESSAGE_2(condition, errorMessage) \
	do \
	{ \
		if (!(condition)) \
		{ \
			FAIL(errorMessage); \
		} \
	} while (false)

// clang-format off
#define _MS_REQUIRE_WITH_MESSAGE_IMPL(_1, _2, NAME, ...) NAME

// clang-format off
#define REQUIRE_WITH_MESSAGE(...) \
	_MS_REQUIRE_WITH_MESSAGE_IMPL(__VA_ARGS__, _MS_REQUIRE_WITH_MESSAGE_2, _MS_REQUIRE_WITH_MESSAGE_1)(__VA_ARGS__)

#endif
