#ifndef MS_RTC_DTLS_TRANSPORT_H
#define MS_RTC_DTLS_TRANSPORT_H

#include "common.h"
#include "RTC/SrtpSession.h"
#include "handles/Timer.h"
#include <string>
#include <map>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <json/json.h>

namespace RTC
{
	class DtlsTransport :
		public Timer::Listener
	{
	public:
		enum class DtlsState
		{
			NEW = 1,
			CONNECTING,
			CONNECTED,
			FAILED,
			CLOSED
		};

	public:
		enum class Role
		{
			NONE = 0,
			AUTO = 1,
			CLIENT,
			SERVER
		};

	public:
		enum class FingerprintAlgorithm
		{
			NONE = 0,
			SHA1 = 1,
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
			RTC::SrtpSession::Profile profile;
			const char* name;
		};

	public:
		class Listener
		{
		public:
			// DTLS is in the process of negotiating a secure connection. Incoming
			// media can flow through.
			// NOTE: The caller MUST NOT call any method during this callback.
			virtual void onDtlsConnecting(DtlsTransport* dtlsTransport) = 0;
			// DTLS has completed negotiation of a secure connection (including DTLS-SRTP
			// and remote fingerprint verification). Outgoing media can now flow through.
			// NOTE: The caller MUST NOT call any method during this callback.
			virtual void onDtlsConnected(DtlsTransport* dtlsTransport, RTC::SrtpSession::Profile srtp_profile, uint8_t* srtp_local_key, size_t srtp_local_key_len, uint8_t* srtp_remote_key, size_t srtp_remote_key_len, std::string& remoteCert) = 0;
			// The DTLS connection has been closed as the result of an error (such as a
			// DTLS alert or a failure to validate the remote fingerprint).
			// NOTE: The caller MUST NOT call Close() during this callback.
			virtual void onDtlsFailed(DtlsTransport* dtlsTransport) = 0;
			// The DTLS connection has been closed due to receipt of a close_notify alert.
			// NOTE: The caller MUST NOT call Close() during this callback.
			virtual void onDtlsClosed(DtlsTransport* dtlsTransport) = 0;
			// Need to send DTLS data to the peer.
			// NOTE: The caller MUST NOT call Close() during this callback.
			virtual void onOutgoingDtlsData(DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) = 0;
			// DTLS application data received.
			// NOTE: The caller MUST NOT call Close() during this callback.
			virtual void onDtlsApplicationData(DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) = 0;
		};

	public:
		static void ClassInit();
		static void ClassDestroy();
		static Json::Value& GetLocalFingerprints();
		static Role StringToRole(std::string role);
		static FingerprintAlgorithm GetFingerprintAlgorithm(std::string fingerprint);
		static bool IsDtls(const uint8_t* data, size_t len);

	private:
		static void GenerateCertificateAndPrivateKey();
		static void ReadCertificateAndPrivateKeyFromFiles();
		static void CreateSSL_CTX();
		static void GenerateFingerprints();

	private:
		static X509* certificate;
		static EVP_PKEY* privateKey;
		static SSL_CTX* sslCtx;
		static uint8_t sslReadBuffer[];
		static std::map<std::string, Role> string2Role;
		static std::map<std::string, FingerprintAlgorithm> string2FingerprintAlgorithm;
		static Json::Value localFingerprints;
		static std::vector<SrtpProfileMapEntry> srtpProfiles;

	public:
		explicit DtlsTransport(Listener* listener);
		virtual ~DtlsTransport();

		void Close();
		void Dump();
		void Run(Role localRole);
		void SetRemoteFingerprint(Fingerprint fingerprint);
		void ProcessDtlsData(const uint8_t* data, size_t len);
		DtlsState GetState();
		Role GetLocalRole();
		void SendApplicationData(const uint8_t* data, size_t len);

	private:
		bool IsRunning();
		void Reset();
		bool CheckStatus(int return_code);
		void SendPendingOutgoingDtlsData();
		bool SetTimeout();
		void ProcessHandshake();
		bool CheckRemoteFingerprint();
		void ExtractSrtpKeys(RTC::SrtpSession::Profile srtp_profile);
		RTC::SrtpSession::Profile GetNegotiatedSrtpProfile();

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
		DtlsState state = DtlsState::NEW;
		Role localRole = Role::NONE;
		Fingerprint remoteFingerprint = { FingerprintAlgorithm::NONE, "" };
		bool handshakeDone = false;
		bool handshakeDoneNow = false;
		std::string remoteCert;
	};

	/* Inline static methods. */

	inline
	Json::Value& DtlsTransport::GetLocalFingerprints()
	{
		return DtlsTransport::localFingerprints;
	}

	inline
	DtlsTransport::Role DtlsTransport::StringToRole(std::string role)
	{
		auto it = DtlsTransport::string2Role.find(role);

		if (it != DtlsTransport::string2Role.end())
			return it->second;
		else
			return DtlsTransport::Role::NONE;
	}

	inline
	DtlsTransport::FingerprintAlgorithm DtlsTransport::GetFingerprintAlgorithm(std::string fingerprint)
	{
		auto it = DtlsTransport::string2FingerprintAlgorithm.find(fingerprint);

		if (it != DtlsTransport::string2FingerprintAlgorithm.end())
			return it->second;
		else
			return DtlsTransport::FingerprintAlgorithm::NONE;
	}

	inline
	bool DtlsTransport::IsDtls(const uint8_t* data, size_t len)
	{
		return (
			// Minimum DTLS record length is 13 bytes.
			(len >= 13) &&
			// DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
			(data[0] > 19 && data[0] < 64)
		);
	}

	/* Inline instance methods. */

	inline
	bool DtlsTransport::IsRunning()
	{
		switch (this->state)
		{
			case DtlsState::NEW:
				return false;
			case DtlsState::CONNECTING:
			case DtlsState::CONNECTED:
				return true;
			case DtlsState::FAILED:
			case DtlsState::CLOSED:
				return false;
		}

		// Make GCC 4.9 happy.
		return false;
	}

	inline
	DtlsTransport::DtlsState DtlsTransport::GetState()
	{
		return this->state;
	}

	inline
	DtlsTransport::Role DtlsTransport::GetLocalRole()
	{
		return this->localRole;
	}
}

#endif
