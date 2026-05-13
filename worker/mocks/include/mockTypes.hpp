#ifndef MS_MOCKS_TYPES_HPP
#define MS_MOCKS_TYPES_HPP

#include <string>

namespace mocks
{
	struct VerificationResult
	{
		bool ok;
		std::string errorMessage;
	};
} // namespace mocks

#endif
