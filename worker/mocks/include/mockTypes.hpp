#ifndef MS_MOCKS_TYPES_HPP
#define MS_MOCKS_TYPES_HPP

#include "common.hpp"
#include <string>

namespace mocks
{
	struct VerificationResult
	{
		bool ok{ false };
		std::string errorMessage;
	};
} // namespace mocks

#endif
