#ifndef MS_RTC_SRTP_SESSION_H
#define MS_RTC_SRTP_SESSION_H

#include "common.h"
#include "DepLibSRTP.h"
#include "RTC/SRTPProfile.h"
#include <srtp/srtp.h>

namespace RTC
{
	class SRTPSession
	{
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
		static __thread MS_BYTE encryptBuffer[];

	public:
		SRTPSession(Type type, RTC::SRTPProfile profile, MS_BYTE* key, size_t key_len);
		~SRTPSession();

		bool EncryptRTP(const MS_BYTE** data, size_t* len);
		bool DecryptSRTP(const MS_BYTE* data, size_t* len);
		bool EncryptRTCP(const MS_BYTE** data, size_t* len);
		bool DecryptSRTCP(const MS_BYTE* data, size_t* len);
		const char* GetLastErrorDesc();
		void Close();

	private:
		// Allocated by this:
		srtp_t session = nullptr;
		// Others:
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
}  // namespace RTC

#endif
