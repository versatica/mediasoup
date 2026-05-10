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
#define REQUIRE_VERIFICATION_RESULT(verificationResult) \
	do \
	{ \
		const auto& result = (verificationResult); \
		\
		if (!(result.ok)) \
		{ \
			FAIL(result.errorMessage); \
		} \
	} while (false)

// clang-format off
#define REQUIRE_WITH_MESSAGE(condition, errorMessage) \
	do \
	{ \
		if (!(condition)) \
		{ \
			FAIL(errorMessage); \
		} \
	} while (false)

#endif
