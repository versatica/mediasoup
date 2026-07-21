#define MS_CLASS "RTC::SrtpSession"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SrtpSession.hpp"
#include "DepLibSRTP.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	/* Static. */

	static constexpr size_t EncryptBufferSize{ 65536 };
	alignas(4) static thread_local uint8_t EncryptBuffer[EncryptBufferSize];

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

		srtp_profile_t profile;

		switch (cryptoSuite)
		{
			case CryptoSuite::AEAD_AES_256_GCM:
			{
				profile = srtp_profile_aead_aes_256_gcm;

				break;
			}

			case CryptoSuite::AEAD_AES_128_GCM:
			{
				profile = srtp_profile_aead_aes_128_gcm;

				break;
			}

			case CryptoSuite::AES_CM_128_HMAC_SHA1_80:
			{
				profile = srtp_profile_aes128_cm_sha1_80;

				break;
			}

			case CryptoSuite::AES_CM_128_HMAC_SHA1_32:
			{
				profile = srtp_profile_aes128_cm_sha1_32;

				break;
			}

			default:
			{
				MS_ABORT("unknown SRTP crypto suite");
			}
		}

		// Create the policy object.
		srtp_policy_t policy{ nullptr };
		srtp_err_status_t err = srtp_policy_create(std::addressof(policy));

		if (DepLibSRTP::IsError(err))
		{
			MS_THROW_ERROR("srtp_policy_create() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
		}

		// Set SSRC.
		srtp_ssrc_t ssrc{};

		switch (type)
		{
			case Type::INBOUND:
			{
				ssrc.type = ssrc_any_inbound;

				break;
			}

			case Type::OUTBOUND:
			{
				ssrc.type = ssrc_any_outbound;

				break;
			}
		}

		ssrc.value = 0;

		err = srtp_policy_set_ssrc(policy, ssrc);

		if (DepLibSRTP::IsError(err))
		{
			srtp_policy_destroy(policy);
			MS_THROW_ERROR("srtp_policy_set_ssrc() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
		}

		// Set the crypto profile.
		err = srtp_policy_set_profile(policy, profile);

		if (DepLibSRTP::IsError(err))
		{
			srtp_policy_destroy(policy);
			MS_THROW_ERROR("srtp_policy_set_profile() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
		}

		// Split the concatenated key+salt buffer using the profile's lengths.
		const size_t masterKeyLen  = srtp_profile_get_master_key_length(profile);
		const size_t masterSaltLen = srtp_profile_get_master_salt_length(profile);

		MS_ASSERT(
		  keyLen == masterKeyLen + masterSaltLen,
		  "given keyLen does not match profile master key + salt length");

		err = srtp_policy_add_key(
		  policy,
		  /*key*/ key,
		  /*key_len*/ masterKeyLen,
		  /*salt*/ key + masterKeyLen,
		  /*salt_len*/ masterSaltLen,
		  /*mki*/ nullptr,
		  /*mki_len*/ 0);

		if (DepLibSRTP::IsError(err))
		{
			srtp_policy_destroy(policy);
			MS_THROW_ERROR("srtp_policy_add_key() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
		}

		// Required for sending RTP retransmission without RTX.
		err = srtp_policy_set_allow_repeat_tx(policy, true);

		if (DepLibSRTP::IsError(err))
		{
			srtp_policy_destroy(policy);
			MS_THROW_ERROR(
			  "srtp_policy_set_allow_repeat_tx() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
		}

		err = srtp_policy_set_window_size(policy, 1024);

		if (DepLibSRTP::IsError(err))
		{
			srtp_policy_destroy(policy);
			MS_THROW_ERROR(
			  "srtp_policy_set_window_size() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
		}

		// Create the SRTP session.
		err = srtp_create(&this->session, policy);

		// Policy is no longer needed once the session is created.
		srtp_policy_destroy(policy);

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
				catch (const std::exception& error) // NOLINT(bugprone-empty-catch)
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
		*data = const_cast<const uint8_t*>(EncryptBuffer);
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
