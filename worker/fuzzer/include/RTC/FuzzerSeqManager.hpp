#ifndef MS_FUZZER_RTC_SEQ_MANAGER_HPP
#define MS_FUZZER_RTC_SEQ_MANAGER_HPP

#include "common.hpp"

namespace FuzzerRtcSeqManager
{
	void Fuzz(const uint8_t* data, size_t len);
}

#endif
