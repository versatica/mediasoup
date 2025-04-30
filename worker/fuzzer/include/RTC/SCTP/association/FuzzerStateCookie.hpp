#ifndef MS_FUZZER_RTC_SCTP_STATE_COOKIE_HPP
#define MS_FUZZER_RTC_SCTP_STATE_COOKIE_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace SCTP
		{
			namespace StateCookie
			{
				void Fuzz(const uint8_t* data, size_t len);
			}
		} // namespace SCTP
	}   // namespace RTC
} // namespace Fuzzer

#endif
