#ifndef MS_CATCH2_MACROS_HPP
#define MS_CATCH2_MACROS_HPP

#include "common.hpp" // IWYU pragma: keep
// NOTE: The macros below expand in the files that include this one, so the
// headers they need are re-exported to them.
// IWYU pragma: begin_exports
#include <catch2/catch_test_macros.hpp>
// IWYU pragma: end_exports

/**
 * `VerificationResult` struct is defined in mocks/include/mockTypes.hpp.
 */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REQUIRE_VERIFICATION_RESULT(verificationResult)                                            \
	do                                                                                               \
	{                                                                                                \
		const auto& result = (verificationResult);                                                     \
                                                                                                   \
		if (!(result.ok))                                                                              \
		{                                                                                              \
			FAIL(result.errorMessage);                                                                   \
		}                                                                                              \
	} while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REQUIRE_WITH_MESSAGE(condition, errorMessage)                                              \
	do                                                                                               \
	{                                                                                                \
		if (!(condition))                                                                              \
		{                                                                                              \
			FAIL(errorMessage);                                                                          \
		}                                                                                              \
	} while (false)

#endif
