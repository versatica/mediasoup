#ifndef MS_FUZZERS_HPP
#define	MS_FUZZERS_HPP

#include "common.hpp"

namespace fuzzers
{
	void fuzzStun(const uint8_t* data, size_t len);
	void fuzzRtp(const uint8_t* data, size_t len);
	void fuzzRtcp(const uint8_t* data, size_t len);
}

#endif

