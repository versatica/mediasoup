#ifndef MS_RTC_SRTP_PROFILE_H
#define MS_RTC_SRTP_PROFILE_H

namespace RTC
{
	enum class SRTPProfile
	{
		NONE                    = 0,
		AES_CM_128_HMAC_SHA1_80 = 1,
		AES_CM_128_HMAC_SHA1_32
	};
}

#endif
