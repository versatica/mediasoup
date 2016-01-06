#define MS_CLASS "RTC::DTLSTransport"

#include "RTC/DTLSTransport.h"
#include "Settings.h"
#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <algorithm>  // std::remove
#include <cstdio>  // std::sprintf(), std::fopen()
#include <cstring>  // std::memcpy(), std::strcmp()
#include <ctime>  // struct timeval
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/asn1.h>

#define MS_SSL_READ_BUFFER_SIZE 65536
// NOTE: Those values are hardcoded as we just use AES_CM_128_HMAC_SHA1_80 and
// AES_CM_128_HMAC_SHA1_32 which share same length values for key and salt.
#define MS_SRTP_MASTER_KEY_LENGTH 16
#define MS_SRTP_MASTER_SALT_LENGTH 14
#define MS_SRTP_MASTER_LENGTH (MS_SRTP_MASTER_KEY_LENGTH + MS_SRTP_MASTER_SALT_LENGTH)
#define LOG_OPENSSL_ERROR(desc)  \
	do  \
	{  \
		if (ERR_peek_error() == 0)  \
			MS_ERROR("OpenSSL error [desc:'%s']", desc);  \
		else  \
		{  \
			unsigned long err;  \
			while ((err = ERR_get_error()) != 0)  \
			{  \
				MS_ERROR("OpenSSL error [desc:'%s', error:'%s']", desc, ERR_error_string(err, nullptr));  \
			}  \
			ERR_clear_error();  \
		}  \
	}  \
	while (0)

/* Static methods for OpenSSL callbacks. */

static inline
int on_ssl_certificate_verify(int preverify_ok, X509_STORE_CTX* ctx)
{
	MS_TRACE();

	// Always valid since DTLS certificates are self-signed.
	return 1;
}

static inline
void on_ssl_info(const SSL* ssl, int where, int ret)
{
	static_cast<RTC::DTLSTransport*>(SSL_get_ex_data(ssl, 0))->onSSLInfo(where, ret);
}

namespace RTC
{
	/* Class variables. */

	X509* DTLSTransport::certificate = nullptr;
	EVP_PKEY* DTLSTransport::privateKey = nullptr;
	SSL_CTX* DTLSTransport::sslCtx = nullptr;
	MS_BYTE DTLSTransport::sslReadBuffer[MS_SSL_READ_BUFFER_SIZE];
	std::map<std::string, DTLSTransport::FingerprintAlgorithm> DTLSTransport::string2FingerprintAlgorithm =
	{
		{ "sha-1",   DTLSTransport::FingerprintAlgorithm::SHA1   },
		{ "sha-224", DTLSTransport::FingerprintAlgorithm::SHA224 },
		{ "sha-256", DTLSTransport::FingerprintAlgorithm::SHA256 },
		{ "sha-384", DTLSTransport::FingerprintAlgorithm::SHA384 },
		{ "sha-512", DTLSTransport::FingerprintAlgorithm::SHA512 }
	};
	std::map<std::string, DTLSTransport::Role> DTLSTransport::string2Role =
	{
		{ "auto",   DTLSTransport::Role::AUTO   },
		{ "client", DTLSTransport::Role::CLIENT },
		{ "server", DTLSTransport::Role::SERVER }
	};
	Json::Value DTLSTransport::localFingerprints = Json::Value(Json::objectValue);
	std::vector<DTLSTransport::SrtpProfileMapEntry> DTLSTransport::srtpProfiles =
	{
		{ RTC::SRTPSession::SRTPProfile::AES_CM_128_HMAC_SHA1_80, "SRTP_AES128_CM_SHA1_80" },
		{ RTC::SRTPSession::SRTPProfile::AES_CM_128_HMAC_SHA1_32, "SRTP_AES128_CM_SHA1_32" }
	};

	/* Class methods. */

	void DTLSTransport::ClassInit()
	{
		MS_TRACE();

		// Generate a X509 certificate and private key (unless PEM files are provided).
		if (Settings::configuration.dtlsCertificateFile.empty() || Settings::configuration.dtlsPrivateKeyFile.empty())
			GenerateCertificateAndPrivateKey();
		else
			ReadCertificateAndPrivateKeyFromFiles();

		// Create a global SSL_CTX.
		CreateSSL_CTX();

		// Generate certificate fingerprints.
		GenerateFingerprints();
	}

	void DTLSTransport::ClassDestroy()
	{
		MS_TRACE();

		if (DTLSTransport::privateKey)
			EVP_PKEY_free(DTLSTransport::privateKey);
		if (DTLSTransport::certificate)
			X509_free(DTLSTransport::certificate);
		if (DTLSTransport::sslCtx)
			SSL_CTX_free(DTLSTransport::sslCtx);
	}

	void DTLSTransport::GenerateCertificateAndPrivateKey()
	{
		MS_TRACE();

		int ret = 0;
		BIGNUM* bne = nullptr;
		RSA* rsa_key = nullptr;
		int num_bits = 1024;
		X509_NAME* cert_name = nullptr;

		// Create a big number object.
		bne = BN_new();
		if (!bne)
		{
			LOG_OPENSSL_ERROR("BN_new() failed");
			goto error;
		}

		ret = BN_set_word(bne, RSA_F4);  // RSA_F4 == 65537.
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("BN_set_word() failed");
			goto error;
		}

		// Generate a RSA key.
		rsa_key = RSA_new();
		if (!rsa_key)
		{
			LOG_OPENSSL_ERROR("RSA_new() failed");
			goto error;
		}

		// This takes some time.
		ret = RSA_generate_key_ex(rsa_key, num_bits, bne, nullptr);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("RSA_generate_key_ex() failed");
			goto error;
		}

		// Create a private key object (needed to hold the RSA key).
		DTLSTransport::privateKey = EVP_PKEY_new();
		if (!DTLSTransport::privateKey)
		{
			LOG_OPENSSL_ERROR("EVP_PKEY_new() failed");
			goto error;
		}

		ret = EVP_PKEY_assign_RSA(DTLSTransport::privateKey, rsa_key);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("EVP_PKEY_assign_RSA() failed");
			goto error;
		}
		// The RSA key now belongs to the private key, so don't clean it up separately.
		rsa_key = nullptr;

		// Create the X509 certificate.
		DTLSTransport::certificate = X509_new();
		if (!DTLSTransport::certificate)
		{
			LOG_OPENSSL_ERROR("X509_new() failed");
			goto error;
		}

		// Set version 3 (note that 0 means version 1).
		X509_set_version(DTLSTransport::certificate, 2);

		// Set serial number (avoid default 0).
		ASN1_INTEGER_set(X509_get_serialNumber(DTLSTransport::certificate), (long)Utils::Crypto::GetRandomUInt(1000000, 9999999));

		// Set valid period.
		X509_gmtime_adj(X509_get_notBefore(DTLSTransport::certificate), -1*60*60*24*365*10);  // - 10 years.
		X509_gmtime_adj(X509_get_notAfter(DTLSTransport::certificate), 60*60*24*365*10);  // 10 years.

		// Set the public key for the certificate using the key.
		ret = X509_set_pubkey(DTLSTransport::certificate, DTLSTransport::privateKey);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("X509_set_pubkey() failed");
			goto error;
		}

		// Set certificate fields.
		cert_name = X509_get_subject_name(DTLSTransport::certificate);
		if (!cert_name)
		{
			LOG_OPENSSL_ERROR("X509_get_subject_name() failed");
			goto error;
		}
		X509_NAME_add_entry_by_txt(cert_name, "O", MBSTRING_ASC, (unsigned char *)MS_APP_NAME, -1, -1, 0);
		X509_NAME_add_entry_by_txt(cert_name, "CN", MBSTRING_ASC, (unsigned char *)MS_APP_NAME, -1, -1, 0);

		// It is self-signed so set the issuer name to be the same as the subject.
		ret = X509_set_issuer_name(DTLSTransport::certificate, cert_name);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("X509_set_issuer_name() failed");
			goto error;
		}

		// Sign the certificate with its own private key.
		ret = X509_sign(DTLSTransport::certificate, DTLSTransport::privateKey, EVP_sha1());
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("X509_sign() failed");
			goto error;
		}

		// Free stuff and return.
		BN_free(bne);
		return;

	error:
		if (bne)
			BN_free(bne);
		if (rsa_key && !DTLSTransport::privateKey)
			RSA_free(rsa_key);
		if (DTLSTransport::privateKey)
			EVP_PKEY_free(DTLSTransport::privateKey);  // NOTE: This also frees the RSA key.
		if (DTLSTransport::certificate)
			X509_free(DTLSTransport::certificate);

		MS_THROW_ERROR("DTLS certificate and private key generation failed");
	}

	void DTLSTransport::ReadCertificateAndPrivateKeyFromFiles()
	{
		MS_TRACE();

		FILE* file = nullptr;

		file = fopen(Settings::configuration.dtlsCertificateFile.c_str(), "r");
		if (!file)
		{
			MS_ERROR("error reading DTLS certificate file: %s", std::strerror(errno));
			goto error;
		}
		DTLSTransport::certificate = PEM_read_X509(file, nullptr, nullptr, nullptr);
		if (!DTLSTransport::certificate)
		{
			LOG_OPENSSL_ERROR("PEM_read_X509() failed");
			goto error;
		}
		fclose(file);

		file = fopen(Settings::configuration.dtlsPrivateKeyFile.c_str(), "r");
		if (!file)
		{
			MS_ERROR("error reading DTLS private key file: %s", std::strerror(errno));
			goto error;
		}
		DTLSTransport::privateKey = PEM_read_PrivateKey(file, nullptr, nullptr, nullptr);
		if (!DTLSTransport::privateKey)
		{
			LOG_OPENSSL_ERROR("PEM_read_PrivateKey() failed");
			goto error;
		}
		fclose(file);

		return;

	error:
		MS_THROW_ERROR("error reading DTLS certificate and private key PEM files");
	}

	void DTLSTransport::CreateSSL_CTX()
	{
		MS_TRACE();

		std::string ssl_srtp_profiles;
		EC_KEY* ecdh = nullptr;
		int ret;

		/* Set the global DTLS context. */

		// - Both DTLS 1.0 and 1.2 (requires OpenSSL >= 1.1.0).
		#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
			DTLSTransport::sslCtx = SSL_CTX_new(DTLS_method());
		// - Just DTLS 1.0 (requires OpenSSL >= 1.0.1).
		#elif (OPENSSL_VERSION_NUMBER >= 0x10001000L)
			DTLSTransport::sslCtx = SSL_CTX_new(DTLSv1_method());
		#else
			#error "too old OpenSSL version"
		#endif

		if (!DTLSTransport::sslCtx)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_new() failed");
			goto error;
		}

		ret = SSL_CTX_use_certificate(DTLSTransport::sslCtx, DTLSTransport::certificate);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_use_certificate() failed");
			goto error;
		}

		ret = SSL_CTX_use_PrivateKey(DTLSTransport::sslCtx, DTLSTransport::privateKey);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_use_PrivateKey() failed");
			goto error;
		}

		ret = SSL_CTX_check_private_key(DTLSTransport::sslCtx);
		if (ret == 0)
		{
			LOG_OPENSSL_ERROR("SSL_CTX_check_private_key() failed");
			goto error;
		}

		// Set options.
		SSL_CTX_set_options(DTLSTransport::sslCtx, SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_NO_TICKET | SSL_OP_SINGLE_ECDH_USE);

		// Don't use sessions cache.
		SSL_CTX_set_session_cache_mode(DTLSTransport::sslCtx, SSL_SESS_CACHE_OFF);

		// Read always as much into the buffer as possible.
		SSL_CTX_set_read_ahead(DTLSTransport::sslCtx, 1);

		SSL_CTX_set_verify_depth(DTLSTransport::sslCtx, 4);

		// Require certificate from peer.
		SSL_CTX_set_verify(DTLSTransport::sslCtx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, on_ssl_certificate_verify);

		// Set SSL info callback.
		SSL_CTX_set_info_callback(DTLSTransport::sslCtx, on_ssl_info);

		// Set ciphers.
		ret = SSL_CTX_set_cipher_list(DTLSTransport::sslCtx, "ALL:!ADH:!LOW:!EXP:!MD5:!aNULL:!eNULL:@STRENGTH");
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
			SSL_CTX_set_ecdh_auto(DTLSTransport::sslCtx, 1);
		#else
			ecdh = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
			if (!ecdh)
			{
				LOG_OPENSSL_ERROR("EC_KEY_new_by_curve_name() failed");
				goto error;
			}
			if (SSL_CTX_set_tmp_ecdh(DTLSTransport::sslCtx, ecdh) != 1)
			{
				LOG_OPENSSL_ERROR("SSL_CTX_set_tmp_ecdh() failed");
				goto error;
			}
			EC_KEY_free(ecdh);
			ecdh = nullptr;
		#endif

		// Set the "use_srtp" DTLS extension.
		for (auto it = DTLSTransport::srtpProfiles.begin(); it != DTLSTransport::srtpProfiles.end(); ++it)
		{
			if (it != DTLSTransport::srtpProfiles.begin())
				ssl_srtp_profiles += ":";

			SrtpProfileMapEntry* profile_entry = &(*it);
			ssl_srtp_profiles += profile_entry->name;
		}
		MS_DEBUG("setting SRTP profiles for DTLS: %s", ssl_srtp_profiles.c_str());
		// NOTE: This function returns 0 on success.
		ret = SSL_CTX_set_tlsext_use_srtp(DTLSTransport::sslCtx, ssl_srtp_profiles.c_str());
		if (ret != 0)
		{
			MS_ERROR("SSL_CTX_set_tlsext_use_srtp() failed when entering '%s'", ssl_srtp_profiles.c_str());
			LOG_OPENSSL_ERROR("SSL_CTX_set_tlsext_use_srtp() failed");
			goto error;
		}

		return;

	error:
		if (DTLSTransport::sslCtx)
		{
			SSL_CTX_free(DTLSTransport::sslCtx);
			DTLSTransport::sslCtx = nullptr;
		}

		if (ecdh)
			EC_KEY_free(ecdh);

		MS_THROW_ERROR("SSL context creation failed");
	}

	void DTLSTransport::GenerateFingerprints()
	{
		MS_TRACE();

		for (auto it = DTLSTransport::string2FingerprintAlgorithm.begin(); it != DTLSTransport::string2FingerprintAlgorithm.end(); ++it)
		{
			std::string algorithm_str = it->first;
			FingerprintAlgorithm algorithm = it->second;
			unsigned char binary_fingerprint[EVP_MAX_MD_SIZE];
			unsigned int size = 0;
			char hex_fingerprint[(EVP_MAX_MD_SIZE * 2) + 1];
			const EVP_MD* hash_function;
			int ret;

			switch (algorithm)
			{
				case FingerprintAlgorithm::SHA1:
					hash_function = EVP_sha1();
					break;
				case FingerprintAlgorithm::SHA224:
					hash_function = EVP_sha224();
					break;
				case FingerprintAlgorithm::SHA256:
					hash_function = EVP_sha256();
					break;
				case FingerprintAlgorithm::SHA384:
					hash_function = EVP_sha384();
					break;
				case FingerprintAlgorithm::SHA512:
					hash_function = EVP_sha512();
					break;
				default:
					MS_ABORT("unknown algorithm");
			}

			ret = X509_digest(DTLSTransport::certificate, hash_function, binary_fingerprint, &size);
			if (ret == 0)
			{
				MS_ERROR("X509_digest() failed");
				MS_THROW_ERROR("Fingerprints generation failed");
			}

			// Convert to hexadecimal format in lowecase without colons.
			for (unsigned int i = 0; i < size; i++)
			{
				std::sprintf(hex_fingerprint + (i * 2), "%.2x", binary_fingerprint[i]);
			}
			hex_fingerprint[size * 2] = '\0';

			MS_DEBUG("%-7s fingerprint: %s", algorithm_str.c_str(), hex_fingerprint);

			// Store in the JSON.
			DTLSTransport::localFingerprints[algorithm_str] = hex_fingerprint;
		}
	}

	/* Instance methods. */

	DTLSTransport::DTLSTransport(Listener* listener) :
		listener(listener)
	{
		MS_TRACE();

		/* Set SSL. */

		this->ssl = SSL_new(DTLSTransport::sslCtx);
		if (!this->ssl)
		{
			LOG_OPENSSL_ERROR("SSL_new() failed");
			goto error;
		}

		// Set this as custom data.
		SSL_set_ex_data(this->ssl, 0, static_cast<void*>(this));

		this->sslBioFromNetwork = BIO_new(BIO_s_mem());
		if (!this->sslBioFromNetwork)
		{
			LOG_OPENSSL_ERROR("BIO_new() failed");
			SSL_free(this->ssl);
			goto error;
		}

		this->sslBioToNetwork = BIO_new(BIO_s_mem());
		if (!this->sslBioToNetwork)
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
		if (this->sslBioFromNetwork)
			BIO_free(this->sslBioFromNetwork);
		if (this->sslBioToNetwork)
			BIO_free(this->sslBioToNetwork);
		if (this->ssl)
			SSL_free(this->ssl);

		// NOTE: If this is not catched by the caller the program will abort, but
		// this should never happen.
		MS_THROW_ERROR("DTLSTransport instance creation failed");
	}

	DTLSTransport::~DTLSTransport()
	{
		MS_TRACE();

		if (this->ssl)
		{
			SSL_free(this->ssl);
			this->ssl = nullptr;
			this->sslBioFromNetwork = nullptr;
			this->sslBioToNetwork = nullptr;
		}
	}

	void DTLSTransport::Close()
	{
		MS_TRACE();

		if (this->running)
		{
			// Send close alert to the peer.
			SSL_shutdown(this->ssl);
			SendPendingOutgoingDTLSData();
		}

		// Close the DTLS timer.
		this->timer->Close();

		// Notify the listener.
		this->listener->onDTLSClosed(this);

		delete this;
	}

	void DTLSTransport::Dump()
	{
		MS_TRACE();

		MS_DEBUG("[role:%s, running:%s, handshake done:%s, connected:%s]",
			(this->localRole == Role::SERVER ? "server" : (this->localRole == Role::CLIENT ? "client" : "none")),
			this->running ? "yes" : "no",
			this->handshakeDone ? "yes" : "no",
			this->connected ? "yes" : "no");
	}

	void DTLSTransport::Run(Role localRole)
	{
		MS_TRACE();

		MS_ASSERT(localRole == Role::CLIENT || localRole == Role::SERVER, "local DTLS role must be 'client' or 'server'");

		Role previousLocalRole = this->localRole;

		if (localRole == previousLocalRole)
		{
			MS_ERROR("same local DTLS role provided, doing nothing");

			return;
		}

		// If the previous local DTLS role was 'client' or 'server' do reset.
		if (previousLocalRole == Role::CLIENT || previousLocalRole == Role::SERVER)
		{
			MS_DEBUG("resetting DTLS due to local role change");

			Reset();
		}

		// Update local role.
		this->localRole = localRole;
		// Set running flag.
		this->running = true;

		// Notify the listener.
		this->listener->onDTLSConnecting(this);

		switch (this->localRole)
		{
			case Role::CLIENT:
				MS_DEBUG("running [role:client]");

				SSL_set_connect_state(this->ssl);
				SSL_do_handshake(this->ssl);
				SendPendingOutgoingDTLSData();
				SetTimeout();
				break;
			case Role::SERVER:
				MS_DEBUG("running [role:server]");

				SSL_set_accept_state(this->ssl);
				SSL_do_handshake(this->ssl);
				break;
			default:
				MS_ABORT("invalid local DTLS role");
				break;
		}
	}

	void DTLSTransport::SetRemoteFingerprint(Fingerprint fingerprint)
	{
		MS_TRACE();

		MS_ASSERT(fingerprint.algorithm != FingerprintAlgorithm::NONE, "no fingerprint algorithm provided");

		this->remoteFingerprint = fingerprint;

		// The remote fingerpring may have been set after DTLS handshake was done,
		// so we may need to process it now.
		if (this->handshakeDone && !this->connected)
		{
			MS_DEBUG("handshake already done, processing it right now");

			ProcessHandshake();
		}
	}

	void DTLSTransport::ProcessDTLSData(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		int written;
		int read;

		if (!this->running)
		{
			MS_ERROR("cannot process data while not running");

			return;
		}

		// Write the received DTLS data into the sslBioFromNetwork.
		written = BIO_write(this->sslBioFromNetwork, (const void*)data, (int)len);
		if (written != (int)len)
			MS_WARN("OpenSSL BIO_write() wrote less (%zu bytes) than given data (%zu bytes)", (size_t)written, len);

		// Must call SSL_read() to process received DTLS data.
		read = SSL_read(this->ssl, (void*)DTLSTransport::sslReadBuffer, MS_SSL_READ_BUFFER_SIZE);

		// Send data if it's ready.
		SendPendingOutgoingDTLSData();

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
				MS_DEBUG("ignoring application data received while DTLS handshake not done");

				return;
			}

			// Notify the listener.
			this->listener->onDTLSApplicationData(this, (MS_BYTE*)DTLSTransport::sslReadBuffer, (size_t)read);
		}
	}

	void DTLSTransport::SendApplicationData(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// We cannot send data to the peer if its remote fingerprint is not validated.
		if (!this->connected)
		{
			MS_ERROR("cannot send application data while DTLS is not fully connected");

			return;
		}

		if (len == 0)
		{
			MS_DEBUG("ignoring 0 length data");

			return;
		}

		int written;

		written = SSL_write(this->ssl, (const void*)data, (int)len);
		if (written < 0)
		{
			LOG_OPENSSL_ERROR("SSL_write() failed");

			CheckStatus(written);
		}
		else if (written != (int)len)
		{
			MS_WARN("OpenSSL SSL_write() wrote less (%d bytes) than given data (%zu bytes)", written, len);
		}

		// Send data.
		SendPendingOutgoingDTLSData();
	}

	void DTLSTransport::Reset()
	{
		MS_TRACE();

		int ret;

		if (!this->running)
			return;

		MS_DEBUG("resetting DTLS transport");

		// Stop the DTLS timer.
		this->timer->Stop();

		// We need to reset the SSL instance so we need to "shutdown" it, but we don't
		// want to send a Close Alert to the peer, so just don't call to
		// SendPendingOutgoingDTLSData().
		SSL_shutdown(this->ssl);

		this->localRole = Role::NONE;
		this->running = false;
		this->handshakeDone = false;
		this->handshakeDoneNow = false;
		this->connected = false;

		// Reset SSL status.
		// NOTE: For this to properly work, SSL_shutdown() must be called before.
		// NOTE: This may fail if not enough DTLS handshake data has been received,
		// but we don't care so just clear the error queue.
		ret = SSL_clear(this->ssl);
		if (ret == 0)
			ERR_clear_error();
	}

	inline
	bool DTLSTransport::CheckStatus(int return_code)
	{
		MS_TRACE();

		int err;

		err = SSL_get_error(this->ssl, return_code);
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
				MS_DEBUG("SSL status: SSL_ERROR_WANT_WRITE");
				break;
			case SSL_ERROR_WANT_X509_LOOKUP:
				MS_DEBUG("SSL status: SSL_ERROR_WANT_X509_LOOKUP");
				break;
			case SSL_ERROR_SYSCALL:
				LOG_OPENSSL_ERROR("SSL status: SSL_ERROR_SYSCALL");
				break;
			case SSL_ERROR_ZERO_RETURN:
				break;
			case SSL_ERROR_WANT_CONNECT:
				MS_DEBUG("SSL status: SSL_ERROR_WANT_CONNECT");
				break;
			case SSL_ERROR_WANT_ACCEPT:
				MS_DEBUG("SSL status: SSL_ERROR_WANT_ACCEPT");
				break;
		}

		// Check if the handshake (or re-handshake) has been done right now.
		if (this->handshakeDoneNow)
		{
			this->handshakeDoneNow = false;
			this->handshakeDone = true;

			// Stop the timer.
			this->timer->Stop();

			// Process the handshake.
			ProcessHandshake();
		}
		// Check if the peer sent close alert or a fatal error happened.
		else if ((SSL_get_shutdown(this->ssl) & SSL_RECEIVED_SHUTDOWN) || err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL)
		{
			bool was_connected = this->connected;

			this->localRole = Role::NONE;
			this->running = false;
			this->handshakeDone = false;
			this->handshakeDoneNow = false;
			this->connected = false;

			if (was_connected)
			{
				MS_DEBUG("DTLS disconnected");

				Reset();

				// Notify to the listener.
				this->listener->onDTLSDisconnected(this);
			}
			else
			{
				MS_DEBUG("DTLS connection failed");

				Reset();

				// Notify to the listener.
				this->listener->onDTLSFailed(this);
			}

			return false;
		}

		return true;
	}

	inline
	void DTLSTransport::SendPendingOutgoingDTLSData()
	{
		MS_TRACE();

		if (BIO_eof(this->sslBioToNetwork))
			return;

		long read;
		char* data = nullptr;

		read = BIO_get_mem_data(this->sslBioToNetwork, &data);

		MS_DEBUG("%ld bytes of DTLS data ready to sent to the peer", read);

		// Notify the listener.
		this->listener->onOutgoingDTLSData(this, (MS_BYTE*)data, (size_t)read);

		// Clear the BIO buffer.
		// NOTE: the (void) avoids the -Wunused-value warning.
		(void)BIO_reset(this->sslBioToNetwork);
	}

	inline
	bool DTLSTransport::SetTimeout()
	{
		MS_TRACE();

		long int ret;
		struct timeval dtls_timeout;
		MS_8BYTES timeout_ms;

		// NOTE: If ret == 0 then ignore the value in dtls_timeout.
		// NOTE: No DTLSv_1_2_get_timeout() or DTLS_get_timeout() in OpenSSL 1.1.0-dev.
		ret = DTLSv1_get_timeout(this->ssl, (void*)&dtls_timeout);
		if (ret == 0)
			return true;

		timeout_ms = (dtls_timeout.tv_sec * (MS_8BYTES)1000) + (dtls_timeout.tv_usec / 1000);
		if (timeout_ms == 0)
		{
			return true;
		}
		else if (timeout_ms < 30000)
		{
			MS_DEBUG("DTLS timer set in %llu ms", (unsigned long long)timeout_ms);

			this->timer->Start(timeout_ms);

			return true;
		}
		// NOTE: Don't start the timer again if the timeout is greater than 30 seconds.
		else
		{
			MS_DEBUG("DTLS timeout too high (%llu ms), resetting DLTS", (unsigned long long)timeout_ms);

			Reset();

			// Notify to the listener.
			this->listener->onDTLSFailed(this);

			return false;
		}
	}

	inline
	void DTLSTransport::ProcessHandshake()
	{
		MS_TRACE();

		MS_ASSERT(this->handshakeDone, "handshake not done yet");

		// If the remote fingerprint is not yet set then do nothing (this method
		// will be called when the fingerprint is set).
		if (this->remoteFingerprint.algorithm == FingerprintAlgorithm::NONE)
		{
			MS_DEBUG("remote fingerprint not yet set, waiting for it");

			return;
		}

		// Validate the remote fingerprint.
		if (!CheckRemoteFingerprint())
		{
			Reset();

			// Notify to the listener.
			this->listener->onDTLSFailed(this);

			return;
		}

		this->connected = true;

		// Get the negotiated SRTP profile.
		RTC::SRTPSession::SRTPProfile srtp_profile;
		srtp_profile = GetNegotiatedSRTPProfile();

		if (srtp_profile != RTC::SRTPSession::SRTPProfile::NONE)
		{
			// Extract the SRTP keys (will notify the listener with them).
			ExtractSRTPKeys(srtp_profile);
		}
		else
		{
			// NOTE: We assume that "use_srtp" DTLS extension is required even if
			// there is no audio/video.
			MS_WARN("SRTP profile not negotiated");

			Reset();

			// Notify to the listener.
			this->listener->onDTLSFailed(this);
		}
	}

	inline
	bool DTLSTransport::CheckRemoteFingerprint()
	{
		MS_TRACE();

		MS_ASSERT(this->remoteFingerprint.algorithm != FingerprintAlgorithm::NONE, "remote fingerprint not set");

		X509* certificate;
		unsigned char binary_fingerprint[EVP_MAX_MD_SIZE];
		unsigned int size = 0;
		char hex_fingerprint[(EVP_MAX_MD_SIZE * 2) + 1];
		const EVP_MD* hash_function;
		int ret;

		certificate = SSL_get_peer_certificate(this->ssl);
		if (!certificate)
		{
			MS_WARN("no certificate was provided by the peer");

			return false;
		}

		switch (this->remoteFingerprint.algorithm)
		{
			case FingerprintAlgorithm::SHA1:
				hash_function = EVP_sha1();
				break;
			case FingerprintAlgorithm::SHA224:
				hash_function = EVP_sha224();
				break;
			case FingerprintAlgorithm::SHA256:
				hash_function = EVP_sha256();
				break;
			case FingerprintAlgorithm::SHA384:
				hash_function = EVP_sha384();
				break;
			case FingerprintAlgorithm::SHA512:
				hash_function = EVP_sha512();
				break;
			default:
				MS_ABORT("unknown algorithm");
		}

		ret = X509_digest(certificate, hash_function, binary_fingerprint, &size);
		X509_free(certificate);
		if (ret == 0)
		{
			MS_ERROR("X509_digest() failed");

			return false;
		}

		// Convert to hexadecimal format in lowecase without colons.
		for (unsigned int i = 0; i < size; i++)
		{
			std::sprintf(hex_fingerprint + (i * 2), "%.2x", binary_fingerprint[i]);
		}
		hex_fingerprint[size * 2] = '\0';

		if (this->remoteFingerprint.value.compare(hex_fingerprint) != 0)
		{
			MS_WARN("fingerprint in the remote certificate (%s) does not match the announced one (%s)", hex_fingerprint, this->remoteFingerprint.value.c_str());

			return false;
		}

		MS_DEBUG("valid remote fingerprint");

		return true;
	}

	inline
	void DTLSTransport::ExtractSRTPKeys(RTC::SRTPSession::SRTPProfile srtp_profile)
	{
		MS_TRACE();

		MS_BYTE srtp_material[MS_SRTP_MASTER_LENGTH * 2];
		MS_BYTE* srtp_local_key;
		MS_BYTE* srtp_local_salt;
		MS_BYTE* srtp_remote_key;
		MS_BYTE* srtp_remote_salt;
		MS_BYTE srtp_local_master_key[MS_SRTP_MASTER_LENGTH];
		MS_BYTE srtp_remote_master_key[MS_SRTP_MASTER_LENGTH];
		int ret;

		ret = SSL_export_keying_material(this->ssl, srtp_material, MS_SRTP_MASTER_LENGTH * 2, "EXTRACTOR-dtls_srtp", 19, nullptr, 0, 0);
		MS_ASSERT(ret != 0, "SSL_export_keying_material() failed");

		switch (this->localRole)
		{
			case Role::SERVER:
				srtp_remote_key = srtp_material;
				srtp_local_key = srtp_remote_key + MS_SRTP_MASTER_KEY_LENGTH;
				srtp_remote_salt = srtp_local_key + MS_SRTP_MASTER_KEY_LENGTH;
				srtp_local_salt = srtp_remote_salt + MS_SRTP_MASTER_SALT_LENGTH;
				break;
			case Role::CLIENT:
				srtp_local_key = srtp_material;
				srtp_remote_key = srtp_local_key + MS_SRTP_MASTER_KEY_LENGTH;
				srtp_local_salt = srtp_remote_key + MS_SRTP_MASTER_KEY_LENGTH;
				srtp_remote_salt = srtp_local_salt + MS_SRTP_MASTER_SALT_LENGTH;
				break;
			default:
				MS_ABORT("no DTLS role set");
				break;
		}

		// Create the SRTP local master key.
		std::memcpy(srtp_local_master_key, srtp_local_key, MS_SRTP_MASTER_KEY_LENGTH);
		std::memcpy(srtp_local_master_key + MS_SRTP_MASTER_KEY_LENGTH, srtp_local_salt, MS_SRTP_MASTER_SALT_LENGTH);
		// Create the SRTP remote master key.
		std::memcpy(srtp_remote_master_key, srtp_remote_key, MS_SRTP_MASTER_KEY_LENGTH);
		std::memcpy(srtp_remote_master_key + MS_SRTP_MASTER_KEY_LENGTH, srtp_remote_salt, MS_SRTP_MASTER_SALT_LENGTH);

		// Notify the listener with the SRTP key material.
		this->listener->onDTLSConnected(this, srtp_profile, srtp_local_master_key, MS_SRTP_MASTER_LENGTH, srtp_remote_master_key, MS_SRTP_MASTER_LENGTH);
	}

	inline
	RTC::SRTPSession::SRTPProfile DTLSTransport::GetNegotiatedSRTPProfile()
	{
		MS_TRACE();

		RTC::SRTPSession::SRTPProfile negotiated_srtp_profile = RTC::SRTPSession::SRTPProfile::NONE;

		// Ensure that the SRTP profile has been negotiated.
		SRTP_PROTECTION_PROFILE* ssl_srtp_profile = SSL_get_selected_srtp_profile(this->ssl);
		if (!ssl_srtp_profile)
		{
			return negotiated_srtp_profile;
		}

		// Get the negotiated SRTP profile.
		for (auto it = DTLSTransport::srtpProfiles.begin(); it != DTLSTransport::srtpProfiles.end(); ++it)
		{
			SrtpProfileMapEntry* profile_entry = &(*it);

			if (std::strcmp(ssl_srtp_profile->name, profile_entry->name) == 0)
			{
				MS_DEBUG("chosen SRTP profile: %s", profile_entry->name);

				negotiated_srtp_profile = profile_entry->profile;
			}
		}

		MS_ASSERT(negotiated_srtp_profile != RTC::SRTPSession::SRTPProfile::NONE, "chosen SRTP profile is not an available one");

		return negotiated_srtp_profile;
	}

	inline
	void DTLSTransport::onSSLInfo(int where, int ret)
	{
		MS_TRACE();

		int w = where & -SSL_ST_MASK;
		const char* role;

		if (w & SSL_ST_CONNECT)     role = "client";
		else if (w & SSL_ST_ACCEPT) role = "server";
		else                        role = "undefined";

		if (where & SSL_CB_LOOP)
		{
			MS_DEBUG("[role:%s, action:'%s']", role, SSL_state_string_long(this->ssl));
		}
		else if (where & SSL_CB_ALERT)
		{
			const char* alert_type;

			switch (*SSL_alert_type_string(ret))
			{
				case 'W':  alert_type = "warning";    break;
				case 'F':  alert_type = "fatal";      break;
				default:   alert_type = "undefined";
			}

			if (where & SSL_CB_READ)
				MS_DEBUG("received DTLS %s alert: %s", alert_type, SSL_alert_desc_string_long(ret));
			else if (where & SSL_CB_WRITE)
				MS_DEBUG("sending DTLS %s alert: %s", alert_type, SSL_alert_desc_string_long(ret));
			else
				MS_DEBUG("DTLS %s alert: %s", alert_type, SSL_alert_desc_string_long(ret));
		}
		else if (where & SSL_CB_EXIT)
		{
			if (ret == 0)
				MS_DEBUG("[role:%s, failed:'%s']", role, SSL_state_string_long(this->ssl));
			else if (ret < 0)
				MS_DEBUG("role: %s, waiting:'%s']", role, SSL_state_string_long(this->ssl));
		}
		else if (where & SSL_CB_HANDSHAKE_START)
		{
			MS_DEBUG("DTLS handshake start");
		}
		else if (where & SSL_CB_HANDSHAKE_DONE)
		{
			MS_DEBUG("DTLS handshake done");

			this->handshakeDoneNow = true;
		}

		// NOTE: checking SSL_get_shutdown(this->ssl) & SSL_RECEIVED_SHUTDOWN here upon
		// receipt of a close alert does not work (the flag is set after this callback).
	}

	inline
	void DTLSTransport::onTimer(Timer* timer)
	{
		MS_TRACE();

		DTLSv1_handle_timeout(this->ssl);

		// If required, send DTLS data.
		SendPendingOutgoingDTLSData();

		// Set the DTLS timer again.
		SetTimeout();
	}
}
