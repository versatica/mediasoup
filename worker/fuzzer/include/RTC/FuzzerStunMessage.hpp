#ifndef MS_FUZZER_RTC_STUN_MESSAGE_HPP
#define	MS_FUZZER_RTC_STUN_MESSAGE_HPP

#include "common.hpp"

namespace Fuzzer
{
	namespace RTC
	{
		namespace StunMessage
		{
			void Fuzz(const uint8_t* data, size_t len);
		}
	}
}

#endif
