#define MS_CLASS "RTC::SRTPSession"

#include "RTC/SRTPSession.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <cstring>  // std::memset(), std::memcpy()
// #include <srtp2/srtp_priv.h>  // TODO: This allows data->stream->ssrc, but it is not public. Wait ofr libsrtp new API.

#define MS_ENCRYPT_BUFFER_SIZE  65536

namespace RTC
{
	/* Class variables. */

	uint8_t SRTPSession::encryptBuffer[MS_ENCRYPT_BUFFER_SIZE];

	/* Class methods. */

	void SRTPSession::ClassInit()
	{
		/* Set libsrtp event handler. */

		// srtp_err_status_t err;

		// err = srtp_install_event_handler((srtp_event_handler_func_t*)onSRTPEvent);
		// if (DepLibSRTP::IsError(err))
		// {
			// MS_THROW_ERROR("srtp_install_event_handler() failed: %s", DepLibSRTP::GetErrorString(err));
		// }
	}

	// void SRTPSession::onSRTPEvent(srtp_event_data_t* data)
	// {
	// 	MS_TRACE();

	// 	const char* event_desc = "";

	// 	switch (data->event)
	// 	{
	// 		case event_ssrc_collision:
	// 			event_desc = "collision occurred";
	// 			break;
	// 		case event_key_soft_limit:
	// 			event_desc = "stream reached the soft key usage limit and will expire soon";
	// 			break;
	// 		case event_key_hard_limit:
	// 			event_desc = "stream reached the hard key usage limit and has expired";
	// 			break;
	// 		case event_packet_index_limit:
	// 			event_desc = "stream reached the hard packet limit (2^48 packets)";
	// 			break;
	// 	}

	// 	// TODO: u64?
	// 	MS_WARN("SSRC %" PRIu64 ": %s", (uint64_t)data->stream->ssrc, event_desc);
	// }

	/* Instance methods. */

	SRTPSession::SRTPSession(Type type, SRTPProfile profile, uint8_t* key, size_t key_len)
	{
		MS_TRACE();

		srtp_err_status_t err;
		srtp_policy_t policy;

		// Set all policy fields to 0.
		std::memset(&policy, 0, sizeof(srtp_policy_t));

		switch (profile)
		{
			case SRTPProfile::AES_CM_128_HMAC_SHA1_80:
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);
				break;
			case SRTPProfile::AES_CM_128_HMAC_SHA1_32:
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy.rtp);
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);  // NOTE: Must be 80 for RTCP!.
				break;
			default:
				MS_ABORT("unknown SRTP suite");
		}

		MS_ASSERT((int)key_len == policy.rtp.cipher_key_len, "given key_len does not match policy.rtp.cipher_key_len");

		switch (type)
		{
			case Type::INBOUND:
				policy.ssrc.type = ssrc_any_inbound;
				break;
			case Type::OUTBOUND:
				policy.ssrc.type = ssrc_any_outbound;
				break;
			default:
				MS_ABORT("unknown SRTPSession::Type");
		}
		policy.ssrc.value = 0;
		policy.key = key;
		policy.allow_repeat_tx = 0;
		policy.window_size = 1024;
		policy.next = nullptr;  // No more policies.

		// Set the SRTP session.
		err = srtp_create(&this->session, &policy);
		if (DepLibSRTP::IsError(err))
		{
			MS_THROW_ERROR("srtp_create() failed: %s", DepLibSRTP::GetErrorString(err));
		}
	}

	SRTPSession::~SRTPSession()
	{
		MS_TRACE();

		if (this->session)
		{
			srtp_err_status_t err;

			err = srtp_dealloc(this->session);
			if (DepLibSRTP::IsError(err))
			{
				MS_ABORT("srtp_dealloc() failed: %s", DepLibSRTP::GetErrorString(err));
			}
		}
	}

	// TODO: It must not memcpy it but instead the provided packet must already be
	// allocated into a buffer.
	bool SRTPSession::EncryptRTP(const uint8_t** data, size_t* len)
	{
		MS_TRACE();

		// Ensure that the resulting SRTP packet fits into the encrypt buffer.
		if (*len + SRTP_MAX_TRAILER_LEN > MS_ENCRYPT_BUFFER_SIZE)
		{
			MS_WARN("cannot encrypt RTP packet, size too big (%zu bytes)", *len);
			this->lastError = srtp_err_status_fail;
			return false;
		}

		std::memcpy(SRTPSession::encryptBuffer, *data, *len);

		this->lastError = srtp_protect(this->session, (void*)SRTPSession::encryptBuffer, (int*)len);
		if (DepLibSRTP::IsError(this->lastError))
		{
			MS_DEBUG("srtp_protect() failed: %s", DepLibSRTP::GetErrorString(this->lastError));
			return false;
		}

		// Update the given data pointer.
		*data = (const uint8_t*)SRTPSession::encryptBuffer;

		return true;
	}

	bool SRTPSession::DecryptSRTP(const uint8_t* data, size_t* len)
	{
		MS_TRACE();

		this->lastError = srtp_unprotect(this->session, (void*)data, (int*)len);
		if (DepLibSRTP::IsError(this->lastError))
		{
			MS_DEBUG("srtp_unprotect() failed: %s", DepLibSRTP::GetErrorString(this->lastError));
			return false;
		}

		return true;
	}

	bool SRTPSession::EncryptRTCP(const uint8_t** data, size_t* len)
	{
		MS_TRACE();

		// Ensure that the resulting SRTCP packet fits into the encrypt buffer.
		if (*len + SRTP_MAX_TRAILER_LEN > MS_ENCRYPT_BUFFER_SIZE)
		{
			MS_WARN("cannot encrypt RTCP packet, size too big (%zu bytes)", *len);
			this->lastError = srtp_err_status_fail;
			return false;
		}

		std::memcpy(SRTPSession::encryptBuffer, *data, *len);

		this->lastError = srtp_protect_rtcp(this->session, (void*)SRTPSession::encryptBuffer, (int*)len);
		if (DepLibSRTP::IsError(this->lastError))
		{
			MS_DEBUG("srtp_protect_rtcp() failed: %s", DepLibSRTP::GetErrorString(this->lastError));
			return false;
		}

		// Update the given data pointer.
		*data = (const uint8_t*)SRTPSession::encryptBuffer;

		return true;
	}

	bool SRTPSession::DecryptSRTCP(const uint8_t* data, size_t* len)
	{
		MS_TRACE();

		this->lastError = srtp_unprotect_rtcp(this->session, (void*)data, (int*)len);
		if (DepLibSRTP::IsError(this->lastError))
		{
			MS_DEBUG("srtp_unprotect_rtcp() failed: %s", DepLibSRTP::GetErrorString(this->lastError));
			return false;
		}

		return true;
	}

	void SRTPSession::Close()
	{
		MS_TRACE();

		delete this;
	}
}
