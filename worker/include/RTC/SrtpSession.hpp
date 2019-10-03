#ifndef MS_RTC_SRTP_SESSION_HPP
#define MS_RTC_SRTP_SESSION_HPP

#include "common.hpp"
#include <srtp.h>

namespace RTC
{
	class SrtpSession
	{
	public:
		enum class Profile
		{
			NONE                    = 0,
			AES_CM_128_HMAC_SHA1_80 = 1,
			AES_CM_128_HMAC_SHA1_32,
			AEAD_AES_256_GCM,
			AEAD_AES_128_GCM
		};

	public:
		enum class Type
		{
			INBOUND = 1,
			OUTBOUND
		};

	public:
		static void ClassInit();

	private:
		static void OnSrtpEvent(srtp_event_data_t* data);

	public:
		SrtpSession(Type type, Profile profile, uint8_t* key, size_t keyLen);
		~SrtpSession();

	public:
		bool EncryptRtp(const uint8_t** data, size_t* len);
		bool DecryptSrtp(uint8_t* data, size_t* len);
		bool EncryptRtcp(const uint8_t** data, size_t* len);
		bool DecryptSrtcp(uint8_t* data, size_t* len);
		void RemoveStream(uint32_t ssrc);

	private:
		// Allocated by this.
		srtp_t session{ nullptr };
	};

	/* Inline instance methods. */

	inline void SrtpSession::RemoveStream(uint32_t ssrc)
	{
		srtp_remove_stream(this->session, uint32_t{ htonl(ssrc) });
	}
} // namespace RTC

#endif
