#ifndef MS_RTC_FINGERPRINT_HASH_H
#define MS_RTC_FINGERPRINT_HASH_H


namespace RTC {


enum class FingerprintHash {
	NONE = 0,
	SHA1 = 1,
	SHA224,
	SHA256,
	SHA384,
	SHA512
};


}  // namespace RTC


#endif
