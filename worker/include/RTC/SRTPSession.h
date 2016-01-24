#ifndef MS_RTC_SRTP_SESSION_H
#define MS_RTC_SRTP_SESSION_H

#include "common.h"
#include "DepLibSRTP.h"
#include <srtp/srtp.h>

namespace RTC
{
	class SRTPSession
	{
	public:
		enum class SRTPProfile
		{
			NONE                    = 0,
			AES_CM_128_HMAC_SHA1_80 = 1,
			AES_CM_128_HMAC_SHA1_32
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
		static void onSRTPEvent(srtp_event_data_t* data);

	private:
		static uint8_t encryptBuffer[];

	public:
		SRTPSession(Type type, SRTPProfile profile, uint8_t* key, size_t key_len);
		~SRTPSession();

		bool EncryptRTP(const uint8_t** data, size_t* len);
		bool DecryptSRTP(const uint8_t* data, size_t* len);
		bool EncryptRTCP(const uint8_t** data, size_t* len);
		bool DecryptSRTCP(const uint8_t* data, size_t* len);
		const char* GetLastErrorDesc();
		void Close();

	private:
		// Allocated by this.
		srtp_t session = nullptr;
		// Others.
		err_status_t lastError = (err_status_t)0;
	};

	/* Inline instance methods. */

	inline
	const char* SRTPSession::GetLastErrorDesc()
	{
		const char* error_string;

		error_string = DepLibSRTP::GetErrorString(this->lastError);
		this->lastError = (err_status_t)0;  // Reset it.

		return error_string;
	}
}

#endif
