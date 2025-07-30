#define MS_CLASS "RTC::SrtpSession"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SrtpSession.hpp"
#include "DepLibSRTP.hpp"
#ifdef MS_LIBURING_SUPPORTED
#include "DepLibUring.hpp"
#endif
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memset()
#include <stdexcept>

namespace RTC
{
	/* Static. */

	static constexpr size_t EncryptBufferSize{ 65536 };
	thread_local static uint8_t EncryptBuffer[EncryptBufferSize];

	/* Class methods. */

	void SrtpSession::ClassInit()
	{
		// Set libsrtp event handler.
		const srtp_err_status_t err =
		  srtp_install_event_handler(static_cast<srtp_event_handler_func_t*>(OnSrtpEvent));

		if (DepLibSRTP::IsError(err))
		{
			MS_THROW_ERROR(
			  "srtp_install_event_handler() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
		}
	}

	FBS::SrtpParameters::SrtpCryptoSuite SrtpSession::CryptoSuiteToFbs(CryptoSuite cryptoSuite)
	{
		switch (cryptoSuite)
		{
			case SrtpSession::CryptoSuite::AEAD_AES_256_GCM:
			{
				return FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_256_GCM;
			}

			case SrtpSession::CryptoSuite::AEAD_AES_128_GCM:
			{
				return FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_128_GCM;
			}

			case SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_80:
			{
				return FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_80;
			}

			case SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_32:
			{
				return FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_32;
			}

				NO_DEFAULT_GCC();
		}
	}

	SrtpSession::CryptoSuite SrtpSession::CryptoSuiteFromFbs(FBS::SrtpParameters::SrtpCryptoSuite cryptoSuite)
	{
		switch (cryptoSuite)
		{
			case FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_256_GCM:
			{
				return SrtpSession::CryptoSuite::AEAD_AES_256_GCM;
			}

			case FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_128_GCM:
			{
				return SrtpSession::CryptoSuite::AEAD_AES_128_GCM;
			}

			case FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_80:
			{
				return SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_80;
			}

			case FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_32:
			{
				return SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_32;
			}

				NO_DEFAULT_GCC();
		}
	}

	void SrtpSession::OnSrtpEvent(srtp_event_data_t* data)
	{
		MS_TRACE();

		switch (data->event)
		{
			case event_ssrc_collision:
			{
				MS_WARN_TAG(srtp, "SSRC collision occurred");

				break;
			}

			case event_key_soft_limit:
			{
				MS_WARN_TAG(srtp, "stream reached the soft key usage limit and will expire soon");

				break;
			}

			case event_key_hard_limit:
			{
				MS_WARN_TAG(srtp, "stream reached the hard key usage limit and has expired");

				break;
			}

			case event_packet_index_limit:
			{
				MS_WARN_TAG(srtp, "stream reached the hard packet limit (2^48 packets)");

				break;
			}
		}
	}

	/* Instance methods. */

	SrtpSession::SrtpSession(Type type, CryptoSuite cryptoSuite, uint8_t* key, size_t keyLen)
	{
		MS_TRACE();

		srtp_policy_t policy; // NOLINT(cppcoreguidelines-pro-type-member-init)

		// Set all policy fields to 0.
		std::memset(&policy, 0, sizeof(srtp_policy_t));

		switch (cryptoSuite)
		{
			case CryptoSuite::AEAD_AES_256_GCM:
			{
				srtp_crypto_policy_set_aes_gcm_256_16_auth(&policy.rtp);
				srtp_crypto_policy_set_aes_gcm_256_16_auth(&policy.rtcp);

				break;
			}

			case CryptoSuite::AEAD_AES_128_GCM:
			{
				srtp_crypto_policy_set_aes_gcm_128_16_auth(&policy.rtp);
				srtp_crypto_policy_set_aes_gcm_128_16_auth(&policy.rtcp);

				break;
			}

			case CryptoSuite::AES_CM_128_HMAC_SHA1_80:
			{
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);

				break;
			}

			case CryptoSuite::AES_CM_128_HMAC_SHA1_32:
			{
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32(&policy.rtp);
				// NOTE: Must be 80 for RTCP.
				srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);

				break;
			}

			default:
			{
				MS_ABORT("unknown SRTP crypto suite");
			}
		}

		MS_ASSERT(
		  keyLen == policy.rtp.cipher_key_len, "given keyLen does not match policy.rtp.cipher_keyLen");

		switch (type)
		{
			case Type::INBOUND:
			{
				policy.ssrc.type = ssrc_any_inbound;

				break;
			}

			case Type::OUTBOUND:
			{
				policy.ssrc.type = ssrc_any_outbound;

				break;
			}
		}

		policy.ssrc.value = 0;
		policy.key        = key;
		// Required for sending RTP retransmission without RTX.
		policy.allow_repeat_tx = 1;
		policy.window_size     = 1024;
		policy.next            = nullptr;

		// Set the SRTP session.
		const srtp_err_status_t err = srtp_create(&this->session, &policy);

		if (DepLibSRTP::IsError(err))
		{
			MS_THROW_ERROR("srtp_create() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
		}
	}

	SrtpSession::~SrtpSession()
	{
		MS_TRACE();

		if (this->session != nullptr)
		{
			const srtp_err_status_t err = srtp_dealloc(this->session);

			if (DepLibSRTP::IsError(err))
			{
				try
				{
					MS_ABORT("srtp_dealloc() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
				}
				catch (const std::exception& error)
				{
					// NOTE: This is to avoid a warning:
					// '~SrtpSession' has a non-throwing exception specification but can
					// still throw [-Wexceptions]
				}
			}
		}
	}

	bool SrtpSession::EncryptRtp(const uint8_t** data, size_t* len)
	{
		MS_TRACE();

		// Ensure that the resulting SRTP packet fits into the encrypt buffer.
		if (*len + SRTP_MAX_TRAILER_LEN > EncryptBufferSize)
		{
			MS_WARN_TAG(srtp, "cannot encrypt RTP packet, size too big (%zu bytes)", *len);

			return false;
		}

		uint8_t* encryptBuffer = EncryptBuffer;
		size_t encryptLen      = EncryptBufferSize;

#ifdef MS_LIBURING_SUPPORTED
		if (DepLibUring::IsEnabled())
		{
			if (!DepLibUring::IsActive())
			{
				goto protect;
			}

			// Use a preallocated buffer, if available.
			auto* sendBuffer = DepLibUring::GetSendBuffer();

			if (sendBuffer)
			{
				encryptBuffer = sendBuffer;
				encryptLen    = DepLibUring::SendBufferSize;
			}
		}

	protect:
#endif

		const srtp_err_status_t err = srtp_protect(
		  /*srtp_t ctx*/ this->session,
		  /*const uint8_t* rtp*/ *data,
		  /*size_t rtp_len*/ *len,
		  /*uint8_t* srtp*/ encryptBuffer,
		  /*size_t* srtp_len*/ std::addressof(encryptLen),
		  /*size_t mki_index*/ 0);

		if (DepLibSRTP::IsError(err))
		{
			MS_WARN_TAG(srtp, "srtp_protect() failed: %s", DepLibSRTP::GetErrorString(err).c_str());

			return false;
		}

		// Update the given data pointer and len.
		*data = const_cast<const uint8_t*>(encryptBuffer);
		*len  = encryptLen;

		return true;
	}

	bool SrtpSession::DecryptSrtp(uint8_t* data, size_t* len)
	{
		MS_TRACE();

		size_t decryptLen = *len;

		const srtp_err_status_t err = srtp_unprotect(
		  /*srtp_t ctx*/ this->session,
		  /*const uint8_t* srtp*/ data,
		  /*size_t srtp_len*/ *len,
		  /*uint8_t* rtp*/ data,
		  /*size_t* rtp_len*/ std::addressof(decryptLen));

		if (DepLibSRTP::IsError(err))
		{
			MS_DEBUG_TAG(srtp, "srtp_unprotect() failed: %s", DepLibSRTP::GetErrorString(err).c_str());

			return false;
		}

		// Update the given len.
		*len = decryptLen;

		return true;
	}

	bool SrtpSession::EncryptRtcp(const uint8_t** data, size_t* len)
	{
		MS_TRACE();

		// Ensure that the resulting SRTCP packet fits into the encrypt buffer.
		if (*len + SRTP_MAX_TRAILER_LEN > EncryptBufferSize)
		{
			MS_WARN_TAG(srtp, "cannot encrypt RTCP packet, size too big (%zu bytes)", *len);

			return false;
		}

		uint8_t* encryptBuffer = EncryptBuffer;
		size_t encryptLen      = EncryptBufferSize;

		const srtp_err_status_t err = srtp_protect_rtcp(
		  /*srtp_t ctx*/ this->session,
		  /*const uint8_t* rtcp*/ *data,
		  /*size_t rtcp_len*/ *len,
		  /*uint8_t* srtcp*/ encryptBuffer,
		  /*size_t* srtcp_len*/ std::addressof(encryptLen),
		  /*size_t mki_index*/ 0);

		if (DepLibSRTP::IsError(err))
		{
			MS_WARN_TAG(srtp, "srtp_protect_rtcp() failed: %s", DepLibSRTP::GetErrorString(err).c_str());

			return false;
		}

		// Update the given data pointer and len.
		*data = (const uint8_t*)EncryptBuffer;
		*len  = encryptLen;

		return true;
	}

	bool SrtpSession::DecryptSrtcp(uint8_t* data, size_t* len)
	{
		MS_TRACE();

		size_t decryptLen = *len;

		const srtp_err_status_t err = srtp_unprotect_rtcp(
		  /*srtp_t ctx*/ this->session,
		  /*const uint8_t* srtcp*/ data,
		  /*size_t srtcp_len*/ *len,
		  /*uint8_t* rtcp*/ data,
		  /*size_t* rtcp_len*/ std::addressof(decryptLen));

		if (DepLibSRTP::IsError(err))
		{
			MS_DEBUG_TAG(srtp, "srtp_unprotect_rtcp() failed: %s", DepLibSRTP::GetErrorString(err).c_str());

			return false;
		}

		// Update the given len.
		*len = decryptLen;

		return true;
	}
} // namespace RTC
