#ifndef MS_RTC_DTLS_TRANSPORT_H
#define MS_RTC_DTLS_TRANSPORT_H

#include "common.h"
#include "RTC/SRTPSession.h"
#include "handles/Timer.h"
#include <string>
#include <map>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <json/json.h>

namespace RTC
{
	class DTLSTransport :
		public Timer::Listener
	{
	public:
		enum class Role
		{
			NONE   = 0,
			AUTO   = 1,
			CLIENT,
			SERVER
		};

	public:
		enum class FingerprintAlgorithm
		{
			NONE    = 0,
			SHA1    = 1,
			SHA224,
			SHA256,
			SHA384,
			SHA512
		};

	public:
		struct Fingerprint
		{
			FingerprintAlgorithm algorithm;
			std::string          value;
		};

	private:
		struct SrtpProfileMapEntry
		{
			RTC::SRTPSession::SRTPProfile profile;
			const char* name;
		};

	public:
		class Listener
		{
		public:
			// NOTE: The caller MUST NOT call Reset() or Close() during the
			// onOutgoingDTLSData() callback.
			virtual void onOutgoingDTLSData(DTLSTransport* dtlsTransport, const MS_BYTE* data, size_t len) = 0;
			// NOTE: The caller MUST NOT call any method during the onDTLSConnected,
			// onDTLSDisconnected or onDTLSFailed callbacks.
			virtual void onDTLSConnected(DTLSTransport* dtlsTransport) = 0;
			virtual void onDTLSDisconnected(DTLSTransport* dtlsTransport) = 0;
			virtual void onDTLSFailed(DTLSTransport* dtlsTransport) = 0;
			virtual void onSRTPKeyMaterial(DTLSTransport* dtlsTransport, RTC::SRTPSession::SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len) = 0;
			virtual void onDTLSApplicationData(DTLSTransport* dtlsTransport, const MS_BYTE* data, size_t len) = 0;
		};

	public:
		static void ClassInit();
		static void ClassDestroy();
		static Json::Value& GetLocalFingerprints();
		static Role GetRole(std::string role);
		static FingerprintAlgorithm GetFingerprintAlgorithm(std::string fingerprint);
		static bool IsDTLS(const MS_BYTE* data, size_t len);

	private:
		static void GenerateCertificateAndPrivateKey();
		static void ReadCertificateAndPrivateKeyFromFiles();
		static void CreateSSL_CTX();
		static void GenerateFingerprints();

	private:
		static X509* certificate;
		static EVP_PKEY* privateKey;
		static SSL_CTX* sslCtx;
		static MS_BYTE sslReadBuffer[];
		static std::map<std::string, Role> string2Role;
		static std::map<std::string, FingerprintAlgorithm> string2FingerprintAlgorithm;
		static Json::Value localFingerprints;
		static std::vector<SrtpProfileMapEntry> srtpProfiles;

	public:
		DTLSTransport(Listener* listener);
		virtual ~DTLSTransport();

		void Start(Role role);
		void SetRemoteFingerprint(Fingerprint fingerprint);
		void Reset();
		void Close();
		void ProcessDTLSData(const MS_BYTE* data, size_t len);
		bool IsStarted();
		bool IsConnected();
		void SendApplicationData(const MS_BYTE* data, size_t len);
		void Dump();

	private:
		bool CheckStatus(int return_code);
		void SendPendingOutgoingDTLSData();
		void SetTimeout();
		void ProcessHandshake();
		bool CheckRemoteFingerprint();
		RTC::SRTPSession::SRTPProfile GetNegotiatedSRTPProfile();
		void ExtractSRTPKeys(RTC::SRTPSession::SRTPProfile srtp_profile);

	/* Callbacks fired by OpenSSL events. */
	public:
		void onSSLInfo(int where, int ret);

	/* Pure virtual methods inherited from Timer::Listener. */
	public:
		virtual void onTimer(Timer* timer) override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Allocated by this.
		SSL* ssl = nullptr;
		BIO* sslBioFromNetwork = nullptr;  // The BIO from which ssl reads.
		BIO* sslBioToNetwork = nullptr;  // The BIO in which ssl writes.
		Timer* timer = nullptr;
		// Others.
		Role localRole = Role::NONE;
		Fingerprint remoteFingerprint = { FingerprintAlgorithm::NONE, "" };
		bool started = false;
		bool handshakeDone = false;
		bool handshakeDoneNow = false;
		bool connected = false;
		bool checkingStatus = false;
		bool doReset = false;
		bool doClose = false;
	};

	/* Inline static methods. */

	inline
	Json::Value& DTLSTransport::GetLocalFingerprints()
	{
		return DTLSTransport::localFingerprints;
	}

	inline
	DTLSTransport::Role DTLSTransport::GetRole(std::string role)
	{
		auto it = DTLSTransport::string2Role.find(role);

		if (it != DTLSTransport::string2Role.end())
			return it->second;
		else
			return DTLSTransport::Role::NONE;
	}

	inline
	DTLSTransport::FingerprintAlgorithm DTLSTransport::GetFingerprintAlgorithm(std::string fingerprint)
	{
		auto it = DTLSTransport::string2FingerprintAlgorithm.find(fingerprint);

		if (it != DTLSTransport::string2FingerprintAlgorithm.end())
			return it->second;
		else
			return DTLSTransport::FingerprintAlgorithm::NONE;
	}

	inline
	bool DTLSTransport::IsDTLS(const MS_BYTE* data, size_t len)
	{
		return (
			// Minimum DTLS record length is 13 bytes.
			(len >= 13) &&
			// DOC: https://tools.ietf.org/html/draft-petithuguenin-avtcore-rfc5764-mux-fixes-00
			(data[0] > 19 && data[0] < 64)
		);
	}

	/* Inline instance methods. */

	inline
	bool DTLSTransport::IsConnected()
	{
		return this->connected;
	}

	inline
	bool DTLSTransport::IsStarted()
	{
		return this->started;
	}
}

#endif
