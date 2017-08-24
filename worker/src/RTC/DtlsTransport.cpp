#define MS_CLASS "RTC::DtlsTransport"
// #define MS_LOG_DEV

#include "RTC/DtlsTransport.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include <openssl/asn1.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <cstdio>  // std::sprintf(), std::fopen()
#include <cstring> // std::memcpy(), std::strcmp()
#include <ctime>   // struct timeval

#define LOG_OPENSSL_ERROR(desc)                                                                    \
	do                                                                                               \
	{                                                                                                \
		if (ERR_peek_error() == 0)                                                                     \
			MS_ERROR("OpenSSL error [desc:'%s']", desc);                                                 \
		else                                                                                           \
		{                                                                                              \
			int64_t err;                                                                                 \
			while ((err = ERR_get_error()) != 0)                                                         \
			{                                                                                            \
				MS_ERROR("OpenSSL error [desc:'%s', error:'%s']", desc, ERR_error_string(err, nullptr));   \
			}                                                                                            \
			ERR_clear_error();                                                                           \
		}                                                                                              \
	} while (false)

/* Static methods for OpenSSL callbacks. */

inline static int onSslCertificateVerify(int /*preverifyOk*/, X509_STORE_CTX* /*ctx*/)
{
	MS_TRACE();

	// Always valid since DTLS certificates are self-signed.
	return 1;
}

inline static void onSslInfo(const SSL* ssl, int where, int ret)
{
	static_cast<RTC::DtlsTransport*>(SSL_get_ex_data(ssl, 0))->OnSslInfo(where, ret);
}

namespace RTC
{
	/* Static. */

	static constexpr int SslReadBufferSize{ 65536 };
	// NOTE: Those values are hardcoded as we just use AES_CM_128_HMAC_SHA1_80 and
	// AES_CM_128_HMAC_SHA1_32 which share same length values for key and salt.
	static constexpr size_t SrtpMasterKeyLength{ 16 };
	static constexpr size_t SrtpMasterSaltLength{ 14 };
	static constexpr size_t SrtpMasterLength{ SrtpMasterKeyLength + SrtpMasterSaltLength };

	/* Class variables. */

	X509* DtlsTransport::certificate{ nullptr };
	EVP_PKEY* DtlsTransport::privateKey{ nullptr };
	SSL_CTX* DtlsTransport::sslCtx{ nullptr };
	uint8_t DtlsTransport::sslReadBuffer[SslReadBufferSize];
	// clang-format off
	std::map<std::string, DtlsTransport::FingerprintAlgorithm> DtlsTransport::string2FingerprintAlgorithm =
	{
		{ "sha-1",   DtlsTransport::FingerprintAlgorithm::SHA1   },
		{ "sha-224", DtlsTransport::FingerprintAlgorithm::SHA224 },
		{ "sha-256", DtlsTransport::FingerprintAlgorithm::SHA256 },
		{ "sha-384", DtlsTransport::FingerprintAlgorithm::SHA384 },
		{ "sha-512", DtlsTransport::FingerprintAlgorithm::SHA512 }
	};
	std::map<std::string, DtlsTransport::Role> DtlsTransport::string2Role =
	{
		{ "auto",   DtlsTransport::Role::AUTO   },
		{ "client", DtlsTransport::Role::CLIENT },
		{ "server", DtlsTransport::Role::SERVER }
	};
	// clang-format on
	Json::Value DtlsTransport::localFingerprints = Json::Value(Json::arrayValue);
	// clang-format off
	std::vector<DtlsTransport::SrtpProfileMapEntry> DtlsTransport::srtpProfiles =
	{
		{ RTC::SrtpSession::Profile::AES_CM_128_HMAC_SHA1_80, "SRTP_AES128_CM_SHA1_80" },
		{ RTC::SrtpSession::Profile::AES_CM_128_HMAC_SHA1_32, "SRTP_AES128_CM_SHA1_32" }
	};
	// clang-format on

	/* Class methods. */

	void DtlsTransport::ClassInit()
	{
		MS_TRACE();

		// Generate a X509 certificate and private key (unless PEM files are provided).
		if (Settings::configuration.dtlsCertificateFile.empty() ||
		    Settings::configuration.dtlsPrivateKeyFile.empty())
		{
			GenerateCertificateAndPrivateKey();
		}
		else
		{
			ReadCertificateAndPrivateKeyFromFiles();
		}

		// Create a global SSL_CTX.
		CreateSslCtx();

		// Generate certificate fingerprints.
		GenerateFingerprints();
	}

	void DtlsTransport::ClassDestroy()
	{
		MS_TRACE();

		if (DtlsTransport::privateKey != nullptr)
			EVP_PKEY_free(DtlsTransport::privateKey);
		if (DtlsTransport::certificate != nullptr)
			X509_free(DtlsTransport::certificate);
		if (DtlsTransport::sslCtx != nullptr)
			SSL_CTX_free(DtlsTransport::sslCtx);
	}

	void DtlsTransport::GenerateCertificateAndPrivateKey()
	{
		MS_TRACE();

		int ret{ 0 };
		BIGNUM* bne{ nullptr };
		RSA* rsaKey{ nullptr };
		int numBits{ 1024 };
		X509_NAME* certName{ nullptr };

		// Create a big number object.
		bne = BN_new();
		if (bne == nullptr)
		{
			LOG_OPENSSL_ERROR("BN_new() failed");
			goto error;
		}

		ret = BN_set_word(bne, RSA_F4); // RSA_F4 == 65537.
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("BN_set_word() failed");
			goto error;
		}

		// Generate a RSA key.
		rsaKey = RSA_new();
		if (rsaKey == nullptr)
		{
			LOG_OPENSSL_ERROR("RSA_new() failed");
			goto error;
		}

		// This takes some time.
		ret = RSA_generate_key_ex(rsaKey, numBits, bne, nullptr);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("RSA_generate_key_ex() failed");
			goto error;
		}

		// Create a private key object (needed to hold the RSA key).
		DtlsTransport::privateKey = EVP_PKEY_new();
		if (DtlsTransport::privateKey == nullptr)
		{
			LOG_OPENSSL_ERROR("EVP_PKEY_new() failed");
			goto error;
		}

		ret = EVP_PKEY_assign_RSA(DtlsTransport::privateKey, rsaKey); // NOLINT
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("EVP_PKEY_assign_RSA() failed");
			goto error;
		}
		// The RSA key now belongs to the private key, so don't clean it up separately.
		rsaKey = nullptr;

		// Create the X509 certificate.
		DtlsTransport::certificate = X509_new();
		if (DtlsTransport::certificate == nullptr)
		{
			LOG_OPENSSL_ERROR("X509_new() failed");
			goto error;
		}

		// Set version 3 (note that 0 means version 1).
		X509_set_version(DtlsTransport::certificate, 2);

		// Set serial number (avoid default 0).
		ASN1_INTEGER_set(
		    X509_get_serialNumber(DtlsTransport::certificate),
		    static_cast<uint64_t>(Utils::Crypto::GetRandomUInt(1000000, 9999999)));

		// Set valid period.
		X509_gmtime_adj(X509_get_notBefore(DtlsTransport::certificate), -315360000); // -10 years.
		X509_gmtime_adj(X509_get_notAfter(DtlsTransport::certificate), 315360000);   // 10 years.

		// Set the public key for the certificate using the key.
		ret = X509_set_pubkey(DtlsTransport::certificate, DtlsTransport::privateKey);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("X509_set_pubkey() failed");
			goto error;
		}

		// Set certificate fields.
		certName = X509_get_subject_name(DtlsTransport::certificate);
		if (certName == nullptr)
		{
			LOG_OPENSSL_ERROR("X509_get_subject_name() failed");
			goto error;
		}
		X509_NAME_add_entry_by_txt(
		    certName, "O", MBSTRING_ASC, reinterpret_cast<const uint8_t*>("mediasoup"), -1, -1, 0);
		X509_NAME_add_entry_by_txt(
		    certName, "CN", MBSTRING_ASC, reinterpret_cast<const uint8_t*>("mediasoup"), -1, -1, 0);

		// It is self-signed so set the issuer name to be the same as the subject.
		ret = X509_set_issuer_name(DtlsTransport::certificate, certName);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("X509_set_issuer_name() failed");
			goto error;
		}

		// Sign the certificate with its own private key.
		ret = X509_sign(DtlsTransport::certificate, DtlsTransport::privateKey, EVP_sha1());
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("X509_sign() failed");
			goto error;
		}

		// Free stuff and return.
		BN_free(bne);

		return;

	error:
		if (bne != nullptr)
			BN_free(bne);
		if ((rsaKey != nullptr) && (DtlsTransport::privateKey == nullptr))
			RSA_free(rsaKey);
		if (DtlsTransport::privateKey != nullptr)
			EVP_PKEY_free(DtlsTransport::privateKey); // NOTE: This also frees the RSA key.
		if (DtlsTransport::certificate != nullptr)
			X509_free(DtlsTransport::certificate);

		MS_THROW_ERROR("DTLS certificate and private key generation failed");
	}

	void DtlsTransport::ReadCertificateAndPrivateKeyFromFiles()
	{
		MS_TRACE();

		FILE* file{ nullptr };

		file = fopen(Settings::configuration.dtlsCertificateFile.c_str(), "r");
		if (file == nullptr)
		{
			MS_ERROR("error reading DTLS certificate file: %s", std::strerror(errno));
			goto error;
		}

		DtlsTransport::certificate = PEM_read_X509(file, nullptr, nullptr, nullptr);
		if (DtlsTransport::certificate == nullptr)
		{
			LOG_OPENSSL_ERROR("PEM_read_X509() failed");
			goto error;
		}

		fclose(file);

		file = fopen(Settings::configuration.dtlsPrivateKeyFile.c_str(), "r");
		if (file == nullptr)
		{
			MS_ERROR("error reading DTLS private key file: %s", std::strerror(errno));
			goto error;
		}

		DtlsTransport::privateKey = PEM_read_PrivateKey(file, nullptr, nullptr, nullptr);
		if (DtlsTransport::privateKey == nullptr)
		{
			LOG_OPENSSL_ERROR("PEM_read_PrivateKey() failed");
			goto error;
		}

		fclose(file);

		return;

	error:
		MS_THROW_ERROR("error reading DTLS certificate and private key PEM files");
	}

	void DtlsTransport::CreateSslCtx()
	{
		MS_TRACE();

		std::string dtlsSrtpProfiles;
		EC_KEY* ecdh{ nullptr };
		int ret;

/* Set the global DTLS context. */

// - Both DTLS 1.0 and 1.2 (requires OpenSSL >= 1.1.0).
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
		DtlsTransport::sslCtx = SSL_CTX_new(DTLS_method());
// - Just DTLS 1.0 (requires OpenSSL >= 1.0.1).
#elif (OPENSSL_VERSION_NUMBER >= 0x10001000L)
		DtlsTransport::sslCtx = SSL_CTX_new(DTLSv1_method());
#else
#error "too old OpenSSL version"
#endif

		if (DtlsTransport::sslCtx == nullptr)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_new() failed");
			goto error;
		}

		ret = SSL_CTX_use_certificate(DtlsTransport::sslCtx, DtlsTransport::certificate);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_use_certificate() failed");
			goto error;
		}

		ret = SSL_CTX_use_PrivateKey(DtlsTransport::sslCtx, DtlsTransport::privateKey);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_use_PrivateKey() failed");
			goto error;
		}

		ret = SSL_CTX_check_private_key(DtlsTransport::sslCtx);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_check_private_key() failed");
			goto error;
		}

		// Set options.
		SSL_CTX_set_options(
		    DtlsTransport::sslCtx,
		    SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_NO_TICKET | SSL_OP_SINGLE_ECDH_USE);

		// Don't use sessions cache.
		SSL_CTX_set_session_cache_mode(DtlsTransport::sslCtx, SSL_SESS_CACHE_OFF);

		// Read always as much into the buffer as possible.
		// NOTE: This is the default for DTLS, but a bug in non latest OpenSSL
		// versions makes this call required.
		SSL_CTX_set_read_ahead(DtlsTransport::sslCtx, 1);

		SSL_CTX_set_verify_depth(DtlsTransport::sslCtx, 4);

		// Require certificate from peer.
		SSL_CTX_set_verify(
		    DtlsTransport::sslCtx,
		    SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
		    onSslCertificateVerify);

		// Set SSL info callback.
		SSL_CTX_set_info_callback(DtlsTransport::sslCtx, onSslInfo);

		// Set ciphers.
		ret = SSL_CTX_set_cipher_list(
		    DtlsTransport::sslCtx, "ALL:!ADH:!LOW:!EXP:!MD5:!aNULL:!eNULL:@STRENGTH");
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_set_cipher_list() failed");
			goto error;
		}

// Enable ECDH ciphers.
// DOC: http://en.wikibooks.org/wiki/OpenSSL/Diffie-Hellman_parameters
// NOTE: https://code.google.com/p/chromium/issues/detail?id=406458
// For OpenSSL >= 1.0.2:
#if (OPENSSL_VERSION_NUMBER >= 0x10002000L)
		SSL_CTX_set_ecdh_auto(DtlsTransport::sslCtx, 1);
#else
		ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

		if (!ecdh)
		{
			LOG_OPENSSL_ERROR("EC_KEY_new_by_curve_name() failed");
			goto error;
		}

		if (SSL_CTX_set_tmp_ecdh(DtlsTransport::sslCtx, ecdh) != 1)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_set_tmp_ecdh() failed");
			goto error;
		}

		EC_KEY_free(ecdh);
		ecdh = nullptr;
#endif

		// Set the "use_srtp" DTLS extension.
		{
			auto it = DtlsTransport::srtpProfiles.begin();
			for (; it != DtlsTransport::srtpProfiles.end(); ++it)
			{
				if (it != DtlsTransport::srtpProfiles.begin())
					dtlsSrtpProfiles += ":";

				SrtpProfileMapEntry* profileEntry = std::addressof(*it);
				dtlsSrtpProfiles += profileEntry->name;
			}
		}

		MS_DEBUG_TAG(dtls, "setting SRTP profiles for DTLS: %s", dtlsSrtpProfiles.c_str());

		// NOTE: This function returns 0 on success.
		ret = SSL_CTX_set_tlsext_use_srtp(DtlsTransport::sslCtx, dtlsSrtpProfiles.c_str());
		if (ret != 0)
		{
			MS_ERROR("SSL_CTX_set_tlsext_use_srtp() failed when entering '%s'", dtlsSrtpProfiles.c_str());
			LOG_OPENSSL_ERROR("SSL_CTX_set_tlsext_use_srtp() failed");
			goto error;
		}

		return;

	error:
		if (DtlsTransport::sslCtx != nullptr)
		{
			SSL_CTX_free(DtlsTransport::sslCtx);
			DtlsTransport::sslCtx = nullptr;
		}

		if (ecdh != nullptr)
			EC_KEY_free(ecdh);

		MS_THROW_ERROR("SSL context creation failed");
	}

	void DtlsTransport::GenerateFingerprints()
	{
		MS_TRACE();

		auto it = DtlsTransport::string2FingerprintAlgorithm.begin();
		for (; it != DtlsTransport::string2FingerprintAlgorithm.end(); ++it)
		{
			std::string algorithmString    = it->first;
			FingerprintAlgorithm algorithm = it->second;
			uint8_t binaryFingerprint[EVP_MAX_MD_SIZE];
			unsigned int size{ 0 };
			char hexFingerprint[(EVP_MAX_MD_SIZE * 3) + 1];
			const EVP_MD* hashFunction;
			int ret;

			switch (algorithm)
			{
				case FingerprintAlgorithm::SHA1:
					hashFunction = EVP_sha1();
					break;
				case FingerprintAlgorithm::SHA224:
					hashFunction = EVP_sha224();
					break;
				case FingerprintAlgorithm::SHA256:
					hashFunction = EVP_sha256();
					break;
				case FingerprintAlgorithm::SHA384:
					hashFunction = EVP_sha384();
					break;
				case FingerprintAlgorithm::SHA512:
					hashFunction = EVP_sha512();
					break;
				default:
					MS_THROW_ERROR("unknown algorithm");
			}

			ret = X509_digest(DtlsTransport::certificate, hashFunction, binaryFingerprint, &size);
			if (ret == 0)
			{
				MS_ERROR("X509_digest() failed");
				MS_THROW_ERROR("Fingerprints generation failed");
			}

			// Convert to hexadecimal format in uppercase with colons.
			for (unsigned int i{ 0 }; i < size; ++i)
			{
				std::sprintf(hexFingerprint + (i * 3), "%.2X:", binaryFingerprint[i]);
			}
			hexFingerprint[(size * 3) - 1] = '\0';

			MS_DEBUG_TAG(dtls, "%-7s fingerprint: %s", algorithmString.c_str(), hexFingerprint);

			// Store in the JSON.

			Json::Value fingerprint(Json::objectValue);

			fingerprint["algorithm"] = algorithmString;
			fingerprint["value"] = hexFingerprint;
			DtlsTransport::localFingerprints.append(fingerprint);
		}
	}

	/* Instance methods. */

	DtlsTransport::DtlsTransport(Listener* listener) : listener(listener)
	{
		MS_TRACE();

		/* Set SSL. */

		this->ssl = SSL_new(DtlsTransport::sslCtx);
		if (this->ssl == nullptr)
		{
			LOG_OPENSSL_ERROR("SSL_new() failed");
			goto error;
		}

		// Set this as custom data.
		SSL_set_ex_data(this->ssl, 0, static_cast<void*>(this));

		this->sslBioFromNetwork = BIO_new(BIO_s_mem());
		if (this->sslBioFromNetwork == nullptr)
		{
			LOG_OPENSSL_ERROR("BIO_new() failed");
			SSL_free(this->ssl);
			goto error;
		}

		this->sslBioToNetwork = BIO_new(BIO_s_mem());
		if (this->sslBioToNetwork == nullptr)
		{
			LOG_OPENSSL_ERROR("BIO_new() failed");
			BIO_free(this->sslBioFromNetwork);
			SSL_free(this->ssl);
			goto error;
		}

		SSL_set_bio(this->ssl, this->sslBioFromNetwork, this->sslBioToNetwork);

		/* Set the DTLS timer. */

		this->timer = new Timer(this);

		return;

	error:
		// NOTE: At this point SSL_set_bio() was not called so we must free BIOs as
		// well.
		if (this->sslBioFromNetwork != nullptr)
			BIO_free(this->sslBioFromNetwork);
		if (this->sslBioToNetwork != nullptr)
			BIO_free(this->sslBioToNetwork);
		if (this->ssl != nullptr)
			SSL_free(this->ssl);

		// NOTE: If this is not catched by the caller the program will abort, but
		// this should never happen.
		MS_THROW_ERROR("DtlsTransport instance creation failed");
	}

	DtlsTransport::~DtlsTransport()
	{
		MS_TRACE();

		if (this->ssl != nullptr)
		{
			SSL_free(this->ssl);
			this->ssl               = nullptr;
			this->sslBioFromNetwork = nullptr;
			this->sslBioToNetwork   = nullptr;
		}
	}

	void DtlsTransport::Destroy()
	{
		MS_TRACE();

		if (IsRunning())
		{
			// Send close alert to the peer.
			SSL_shutdown(this->ssl);
			SendPendingOutgoingDtlsData();
		}

		// Destroy the DTLS timer.
		this->timer->Destroy();

		delete this;
	}

	void DtlsTransport::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<DtlsTransport>");
		MS_DUMP(
		    "  [role:%s, running:%s, handshake done:%s, connected:%s]",
		    (this->localRole == Role::SERVER ? "server"
		                                     : (this->localRole == Role::CLIENT ? "client" : "none")),
		    IsRunning() ? "yes" : "no",
		    this->handshakeDone ? "yes" : "no",
		    this->state == DtlsState::CONNECTED ? "yes" : "no");
		MS_DUMP("</DtlsTransport>");
	}

	void DtlsTransport::Run(Role localRole)
	{
		MS_TRACE();

		MS_ASSERT(
		    localRole == Role::CLIENT || localRole == Role::SERVER,
		    "local DTLS role must be 'client' or 'server'");

		Role previousLocalRole = this->localRole;

		if (localRole == previousLocalRole)
		{
			MS_ERROR("same local DTLS role provided, doing nothing");

			return;
		}

		// If the previous local DTLS role was 'client' or 'server' do reset.
		if (previousLocalRole == Role::CLIENT || previousLocalRole == Role::SERVER)
		{
			MS_DEBUG_TAG(dtls, "resetting DTLS due to local role change");

			Reset();
		}

		// Update local role.
		this->localRole = localRole;

		// Set state and notify the listener.
		this->state = DtlsState::CONNECTING;
		this->listener->OnDtlsConnecting(this);

		switch (this->localRole)
		{
			case Role::CLIENT:
				MS_DEBUG_TAG(dtls, "running [role:client]");

				SSL_set_connect_state(this->ssl);
				SSL_do_handshake(this->ssl);
				SendPendingOutgoingDtlsData();
				SetTimeout();
				break;

			case Role::SERVER:
				MS_DEBUG_TAG(dtls, "running [role:server]");

				SSL_set_accept_state(this->ssl);
				SSL_do_handshake(this->ssl);
				break;

			default:
				MS_ABORT("invalid local DTLS role");
		}
	}

	void DtlsTransport::SetRemoteFingerprint(Fingerprint fingerprint)
	{
		MS_TRACE();

		MS_ASSERT(
		    fingerprint.algorithm != FingerprintAlgorithm::NONE, "no fingerprint algorithm provided");

		this->remoteFingerprint = fingerprint;

		// The remote fingerpring may have been set after DTLS handshake was done,
		// so we may need to process it now.
		if (this->handshakeDone && this->state != DtlsState::CONNECTED)
		{
			MS_DEBUG_TAG(dtls, "handshake already done, processing it right now");

			ProcessHandshake();
		}
	}

	void DtlsTransport::ProcessDtlsData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		int written;
		int read;

		if (!IsRunning())
		{
			MS_ERROR("cannot process data while not running");

			return;
		}

		// Write the received DTLS data into the sslBioFromNetwork.
		written = BIO_write(this->sslBioFromNetwork, (const void*)data, static_cast<int>(len));
		if (written != static_cast<int>(len))
		{
			MS_WARN_TAG(
			    dtls,
			    "OpenSSL BIO_write() wrote less (%zu bytes) than given data (%zu bytes)",
			    static_cast<size_t>(written),
			    len);
		}

		// Must call SSL_read() to process received DTLS data.
		read = SSL_read(this->ssl, (void*)DtlsTransport::sslReadBuffer, SslReadBufferSize);

		// Send data if it's ready.
		SendPendingOutgoingDtlsData();

		// Check SSL status and return if it is bad/closed.
		if (!CheckStatus(read))
			return;

		// Set/update the DTLS timeout.
		if (!SetTimeout())
			return;

		// Application data received. Notify to the listener.
		if (read > 0)
		{
			// It is allowed to receive DTLS data even before validating remote fingerprint.
			if (!this->handshakeDone)
			{
				MS_WARN_TAG(dtls, "ignoring application data received while DTLS handshake not done");

				return;
			}

			// Notify the listener.
			this->listener->OnDtlsApplicationData(
			    this, (uint8_t*)DtlsTransport::sslReadBuffer, static_cast<size_t>(read));
		}
	}

	void DtlsTransport::SendApplicationData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// We cannot send data to the peer if its remote fingerprint is not validated.
		if (this->state != DtlsState::CONNECTED)
		{
			MS_WARN_TAG(dtls, "cannot send application data while DTLS is not fully connected");

			return;
		}

		if (len == 0)
		{
			MS_WARN_TAG(dtls, "ignoring 0 length data");

			return;
		}

		int written;

		written = SSL_write(this->ssl, (const void*)data, static_cast<int>(len));
		if (written < 0)
		{
			LOG_OPENSSL_ERROR("SSL_write() failed");

			CheckStatus(written);
		}
		else if (written != static_cast<int>(len))
		{
			MS_WARN_TAG(
			    dtls, "OpenSSL SSL_write() wrote less (%d bytes) than given data (%zu bytes)", written, len);
		}

		// Send data.
		SendPendingOutgoingDtlsData();
	}

	void DtlsTransport::Reset()
	{
		MS_TRACE();

		int ret;

		if (!IsRunning())
			return;

		MS_WARN_TAG(dtls, "resetting DTLS transport");

		// Stop the DTLS timer.
		this->timer->Stop();

		// We need to reset the SSL instance so we need to "shutdown" it, but we don't
		// want to send a Close Alert to the peer, so just don't call to
		// SendPendingOutgoingDTLSData().
		SSL_shutdown(this->ssl);

		this->localRole        = Role::NONE;
		this->state            = DtlsState::NEW;
		this->handshakeDone    = false;
		this->handshakeDoneNow = false;

		// Reset SSL status.
		// NOTE: For this to properly work, SSL_shutdown() must be called before.
		// NOTE: This may fail if not enough DTLS handshake data has been received,
		// but we don't care so just clear the error queue.
		ret = SSL_clear(this->ssl);
		if (ret == 0)
			ERR_clear_error();
	}

	inline bool DtlsTransport::CheckStatus(int returnCode)
	{
		MS_TRACE();

		int err;
		bool wasHandshakeDone = this->handshakeDone;

		err = SSL_get_error(this->ssl, returnCode);
		switch (err)
		{
			case SSL_ERROR_NONE:
				break;
			case SSL_ERROR_SSL:
				LOG_OPENSSL_ERROR("SSL status: SSL_ERROR_SSL");
				break;
			case SSL_ERROR_WANT_READ:
				break;
			case SSL_ERROR_WANT_WRITE:
				MS_WARN_TAG(dtls, "SSL status: SSL_ERROR_WANT_WRITE");
				break;
			case SSL_ERROR_WANT_X509_LOOKUP:
				MS_DEBUG_TAG(dtls, "SSL status: SSL_ERROR_WANT_X509_LOOKUP");
				break;
			case SSL_ERROR_SYSCALL:
				LOG_OPENSSL_ERROR("SSL status: SSL_ERROR_SYSCALL");
				break;
			case SSL_ERROR_ZERO_RETURN:
				break;
			case SSL_ERROR_WANT_CONNECT:
				MS_WARN_TAG(dtls, "SSL status: SSL_ERROR_WANT_CONNECT");
				break;
			case SSL_ERROR_WANT_ACCEPT:
				MS_WARN_TAG(dtls, "SSL status: SSL_ERROR_WANT_ACCEPT");
				break;
			default:
				MS_WARN_TAG(dtls, "SSL status: unknown error");
		}

		// Check if the handshake (or re-handshake) has been done right now.
		if (this->handshakeDoneNow)
		{
			this->handshakeDoneNow = false;
			this->handshakeDone    = true;

			// Stop the timer.
			this->timer->Stop();

			// Process the handshake just once (ignore if DTLS renegotiation).
			if (!wasHandshakeDone)
				ProcessHandshake();
		}
		// Check if the peer sent close alert or a fatal error happened.
		else if (((SSL_get_shutdown(this->ssl) & SSL_RECEIVED_SHUTDOWN) != 0) || err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL)
		{
			if (this->state == DtlsState::CONNECTED)
			{
				MS_DEBUG_TAG(dtls, "disconnected");

				Reset();

				// Set state and notify the listener.
				this->state = DtlsState::CLOSED;
				this->listener->OnDtlsClosed(this);
			}
			else
			{
				MS_WARN_TAG(dtls, "connection failed");

				Reset();

				// Set state and notify the listener.
				this->state = DtlsState::FAILED;
				this->listener->OnDtlsFailed(this);
			}

			return false;
		}

		return true;
	}

	inline void DtlsTransport::SendPendingOutgoingDtlsData()
	{
		MS_TRACE();

		if (BIO_eof(this->sslBioToNetwork))
			return;

		int64_t read;
		char* data{ nullptr };

		read = BIO_get_mem_data(this->sslBioToNetwork, &data); // NOLINT
		if (read <= 0)
			return;

		MS_DEBUG_DEV("%ld bytes of DTLS data ready to sent to the peer", read);

		// Notify the listener.
		this->listener->OnOutgoingDtlsData(
		    this, reinterpret_cast<uint8_t*>(data), static_cast<size_t>(read));

		// Clear the BIO buffer.
		// NOTE: the (void) avoids the -Wunused-value warning.
		(void)BIO_reset(this->sslBioToNetwork);
	}

	inline bool DtlsTransport::SetTimeout()
	{
		MS_TRACE();

		int64_t ret;
		// clang-format off
		struct timeval dtlsTimeout{};
		// clang-format on
		uint64_t timeoutMs;

		// NOTE: If ret == 0 then ignore the value in dtlsTimeout.
		// NOTE: No DTLSv_1_2_get_timeout() or DTLS_get_timeout() in OpenSSL 1.1.0-dev.
		ret = DTLSv1_get_timeout(this->ssl, (void*)&dtlsTimeout); // NOLINT
		if (ret == 0)
			return true;

		timeoutMs = (dtlsTimeout.tv_sec * static_cast<uint64_t>(1000)) + (dtlsTimeout.tv_usec / 1000);
		if (timeoutMs == 0)
		{
			return true;
		}
		if (timeoutMs < 30000)
		{
			MS_DEBUG_DEV("DTLS timer set in %" PRIu64 "ms", timeoutMs);

			this->timer->Start(timeoutMs);

			return true;
		}
		// NOTE: Don't start the timer again if the timeout is greater than 30 seconds.

		MS_WARN_TAG(dtls, "DTLS timeout too high (%" PRIu64 "ms), resetting DLTS", timeoutMs);

		Reset();

		// Set state and notify the listener.
		this->state = DtlsState::FAILED;
		this->listener->OnDtlsFailed(this);

		return false;
	}

	inline void DtlsTransport::ProcessHandshake()
	{
		MS_TRACE();

		MS_ASSERT(this->handshakeDone, "handshake not done yet");

		// If the remote fingerprint is not yet set then do nothing (this method
		// will be called when the fingerprint is set).
		if (this->remoteFingerprint.algorithm == FingerprintAlgorithm::NONE)
		{
			MS_DEBUG_TAG(dtls, "remote fingerprint not yet set, waiting for it");

			return;
		}

		// Validate the remote fingerprint.
		if (!CheckRemoteFingerprint())
		{
			Reset();

			// Set state and notify the listener.
			this->state = DtlsState::FAILED;
			this->listener->OnDtlsFailed(this);

			return;
		}

		// Get the negotiated SRTP profile.
		RTC::SrtpSession::Profile srtpProfile;
		srtpProfile = GetNegotiatedSrtpProfile();

		if (srtpProfile != RTC::SrtpSession::Profile::NONE)
		{
			// Extract the SRTP keys (will notify the listener with them).
			ExtractSrtpKeys(srtpProfile);
		}
		else
		{
			// NOTE: We assume that "use_srtp" DTLS extension is required even if
			// there is no audio/video.
			MS_WARN_TAG(dtls, "SRTP profile not negotiated");

			Reset();

			// Set state and notify the listener.
			this->state = DtlsState::FAILED;
			this->listener->OnDtlsFailed(this);
		}
	}

	inline bool DtlsTransport::CheckRemoteFingerprint()
	{
		MS_TRACE();

		MS_ASSERT(
		    this->remoteFingerprint.algorithm != FingerprintAlgorithm::NONE,
		    "remote fingerprint not set");

		X509* certificate;
		uint8_t binaryFingerprint[EVP_MAX_MD_SIZE];
		unsigned int size{ 0 };
		char hexFingerprint[(EVP_MAX_MD_SIZE * 3) + 1];
		const EVP_MD* hashFunction;
		int ret;

		certificate = SSL_get_peer_certificate(this->ssl);
		if (certificate == nullptr)
		{
			MS_WARN_TAG(dtls, "no certificate was provided by the peer");

			return false;
		}

		switch (this->remoteFingerprint.algorithm)
		{
			case FingerprintAlgorithm::SHA1:
				hashFunction = EVP_sha1();
				break;
			case FingerprintAlgorithm::SHA224:
				hashFunction = EVP_sha224();
				break;
			case FingerprintAlgorithm::SHA256:
				hashFunction = EVP_sha256();
				break;
			case FingerprintAlgorithm::SHA384:
				hashFunction = EVP_sha384();
				break;
			case FingerprintAlgorithm::SHA512:
				hashFunction = EVP_sha512();
				break;
			default:
				MS_ABORT("unknown algorithm");
		}

		// Compare the remote fingerprint with the value given via signaling.
		ret = X509_digest(certificate, hashFunction, binaryFingerprint, &size);
		if (ret == 0)
		{
			MS_ERROR("X509_digest() failed");

			X509_free(certificate);

			return false;
		}

		// Convert to hexadecimal format in uppercase with colons.
		for (unsigned int i{ 0 }; i < size; ++i)
		{
			std::sprintf(hexFingerprint + (i * 3), "%.2X:", binaryFingerprint[i]);
		}
		hexFingerprint[(size * 3) - 1] = '\0';

		if (this->remoteFingerprint.value != hexFingerprint)
		{
			MS_WARN_TAG(
			    dtls,
			    "fingerprint in the remote certificate (%s) does not match the announced one (%s)",
			    hexFingerprint,
			    this->remoteFingerprint.value.c_str());

			X509_free(certificate);

			return false;
		}

		MS_DEBUG_TAG(dtls, "valid remote fingerprint");

		// Get the remote certificate in PEM format.

		BIO* bio = BIO_new(BIO_s_mem());

		// Ensure the underlying BUF_MEM structure is also freed.
		// NOTE: Avoid stupid "warning: value computed is not used [-Wunused-value]" since
		// BIO_set_close() always returns 1.
		(void)BIO_set_close(bio, BIO_CLOSE);

		ret = PEM_write_bio_X509(bio, certificate);
		if (ret != 1)
		{
			LOG_OPENSSL_ERROR("PEM_write_bio_X509() failed");

			X509_free(certificate);
			BIO_free(bio);

			return false;
		}

		BUF_MEM* mem;

		BIO_get_mem_ptr(bio, &mem); // NOLINT
		if ((mem == nullptr) || (mem->data == nullptr) || (mem->length == 0u))
		{
			LOG_OPENSSL_ERROR("BIO_get_mem_ptr() failed");

			X509_free(certificate);
			BIO_free(bio);

			return false;
		}

		this->remoteCert = std::string(mem->data, mem->length);

		X509_free(certificate);
		BIO_free(bio);

		return true;
	}

	inline void DtlsTransport::ExtractSrtpKeys(RTC::SrtpSession::Profile srtpProfile)
	{
		MS_TRACE();

		uint8_t srtpMaterial[SrtpMasterLength * 2];
		uint8_t* srtpLocalKey;
		uint8_t* srtpLocalSalt;
		uint8_t* srtpRemoteKey;
		uint8_t* srtpRemoteSalt;
		uint8_t srtpLocalMasterKey[SrtpMasterLength];
		uint8_t srtpRemoteMasterKey[SrtpMasterLength];
		int ret;

		ret = SSL_export_keying_material(
		    this->ssl, srtpMaterial, SrtpMasterLength * 2, "EXTRACTOR-dtls_srtp", 19, nullptr, 0, 0);

		MS_ASSERT(ret != 0, "SSL_export_keying_material() failed");

		switch (this->localRole)
		{
			case Role::SERVER:
				srtpRemoteKey  = srtpMaterial;
				srtpLocalKey   = srtpRemoteKey + SrtpMasterKeyLength;
				srtpRemoteSalt = srtpLocalKey + SrtpMasterKeyLength;
				srtpLocalSalt  = srtpRemoteSalt + SrtpMasterSaltLength;
				break;
			case Role::CLIENT:
				srtpLocalKey   = srtpMaterial;
				srtpRemoteKey  = srtpLocalKey + SrtpMasterKeyLength;
				srtpLocalSalt  = srtpRemoteKey + SrtpMasterKeyLength;
				srtpRemoteSalt = srtpLocalSalt + SrtpMasterSaltLength;
				break;
			default:
				MS_ABORT("no DTLS role set");
				break;
		}

		// Create the SRTP local master key.
		std::memcpy(srtpLocalMasterKey, srtpLocalKey, SrtpMasterKeyLength);
		std::memcpy(srtpLocalMasterKey + SrtpMasterKeyLength, srtpLocalSalt, SrtpMasterSaltLength);
		// Create the SRTP remote master key.
		std::memcpy(srtpRemoteMasterKey, srtpRemoteKey, SrtpMasterKeyLength);
		std::memcpy(srtpRemoteMasterKey + SrtpMasterKeyLength, srtpRemoteSalt, SrtpMasterSaltLength);

		// Set state and notify the listener.
		this->state = DtlsState::CONNECTED;
		this->listener->OnDtlsConnected(
		    this,
		    srtpProfile,
		    srtpLocalMasterKey,
		    SrtpMasterLength,
		    srtpRemoteMasterKey,
		    SrtpMasterLength,
		    this->remoteCert);
	}

	inline RTC::SrtpSession::Profile DtlsTransport::GetNegotiatedSrtpProfile()
	{
		MS_TRACE();

		RTC::SrtpSession::Profile negotiatedSrtpProfile = RTC::SrtpSession::Profile::NONE;

		// Ensure that the SRTP profile has been negotiated.
		SRTP_PROTECTION_PROFILE* sslSrtpProfile = SSL_get_selected_srtp_profile(this->ssl);
		if (sslSrtpProfile == nullptr)
		{
			return negotiatedSrtpProfile;
		}

		// Get the negotiated SRTP profile.
		auto it = DtlsTransport::srtpProfiles.begin();
		for (; it != DtlsTransport::srtpProfiles.end(); ++it)
		{
			SrtpProfileMapEntry* profileEntry = std::addressof(*it);

			if (std::strcmp(sslSrtpProfile->name, profileEntry->name) == 0)
			{
				MS_DEBUG_TAG(dtls, "chosen SRTP profile: %s", profileEntry->name);

				negotiatedSrtpProfile = profileEntry->profile;
			}
		}

		MS_ASSERT(
		    negotiatedSrtpProfile != RTC::SrtpSession::Profile::NONE,
		    "chosen SRTP profile is not an available one");

		return negotiatedSrtpProfile;
	}

	inline void DtlsTransport::OnSslInfo(int where, int ret)
	{
		MS_TRACE();

		int w = where & -SSL_ST_MASK;
		const char* role;

		if ((w & SSL_ST_CONNECT) != 0)
			role = "client";
		else if ((w & SSL_ST_ACCEPT) != 0)
			role = "server";
		else
			role = "undefined";

		if ((where & SSL_CB_LOOP) != 0)
		{
			MS_DEBUG_TAG(dtls, "[role:%s, action:'%s']", role, SSL_state_string_long(this->ssl));
		}
		else if ((where & SSL_CB_ALERT) != 0)
		{
			const char* alertType;

			switch (*SSL_alert_type_string(ret))
			{
				case 'W':
					alertType = "warning";
					break;
				case 'F':
					alertType = "fatal";
					break;
				default:
					alertType = "undefined";
			}

			if ((where & SSL_CB_READ) != 0)
			{
				MS_WARN_TAG(dtls, "received DTLS %s alert: %s", alertType, SSL_alert_desc_string_long(ret));
			}
			else if ((where & SSL_CB_WRITE) != 0)
			{
				MS_DEBUG_TAG(dtls, "sending DTLS %s alert: %s", alertType, SSL_alert_desc_string_long(ret));
			}
			else
			{
				MS_DEBUG_TAG(dtls, "DTLS %s alert: %s", alertType, SSL_alert_desc_string_long(ret));
			}
		}
		else if ((where & SSL_CB_EXIT) != 0)
		{
			if (ret == 0)
				MS_DEBUG_TAG(dtls, "[role:%s, failed:'%s']", role, SSL_state_string_long(this->ssl));
			else if (ret < 0)
				MS_DEBUG_TAG(dtls, "role: %s, waiting:'%s']", role, SSL_state_string_long(this->ssl));
		}
		else if ((where & SSL_CB_HANDSHAKE_START) != 0)
		{
			MS_DEBUG_TAG(dtls, "DTLS handshake start");
		}
		else if ((where & SSL_CB_HANDSHAKE_DONE) != 0)
		{
			MS_DEBUG_TAG(dtls, "DTLS handshake done");

			this->handshakeDoneNow = true;
		}

		// NOTE: checking SSL_get_shutdown(this->ssl) & SSL_RECEIVED_SHUTDOWN here upon
		// receipt of a close alert does not work (the flag is set after this callback).
	}

	inline void DtlsTransport::OnTimer(Timer* /*timer*/)
	{
		MS_TRACE();

		DTLSv1_handle_timeout(this->ssl);
		// If required, send DTLS data.
		SendPendingOutgoingDtlsData();
		// Set the DTLS timer again.
		SetTimeout();
	}
} // namespace RTC
