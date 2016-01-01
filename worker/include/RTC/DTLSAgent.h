#ifndef MS_RTC_DTLS_AGENT_H
#define MS_RTC_DTLS_AGENT_H

#include "common.h"
#include "handles/Timer.h"
#include "RTC/DTLSRole.h"
#include "RTC/FingerprintHash.h"
#include "RTC/SRTPProfile.h"
#include <string>
#include <map>
#include <vector>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/x509.h>

namespace RTC
{
	class DTLSAgent :
		public Timer::Listener
	{
	private:
		struct SrtpProfileMapEntry
		{
			SRTPProfile profile;
			const char* name;
		};

	public:
		class Listener
		{
		public:
			// NOTE: The caller MUST NOT call Reset() or Close() during the
			// onOutgoingDTLSData() callback.
			virtual void onOutgoingDTLSData(DTLSAgent* dtlsAgent, const MS_BYTE* data, size_t len) = 0;
			// NOTE: The caller MUST NOT call any method during the onDTLSConnected,
			// onDTLSDisconnected or onDTLSFailed callbacks.
			virtual void onDTLSConnected(DTLSAgent* dtlsAgent) = 0;
			virtual void onDTLSDisconnected(DTLSAgent* dtlsAgent) = 0;
			virtual void onDTLSFailed(DTLSAgent* dtlsAgent) = 0;
			virtual void onSRTPKeyMaterial(DTLSAgent* dtlsAgent, RTC::SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len) = 0;
			virtual void onDTLSApplicationData(DTLSAgent* dtlsAgent, const MS_BYTE* data, size_t len) = 0;
		};

	public:
		static void ClassInit();
		static void ClassDestroy();
		static bool IsDTLS(const MS_BYTE* data, size_t len);
		static const std::string& GetFingerprint(RTC::FingerprintHash hash);

	private:
		static void GenerateCertificateAndPrivateKey();
		static void ReadCertificateAndPrivateKeyFromFiles();
		static void CreateSSL_CTX();
		static void GenerateFingerprints();

	private:
		static X509* certificate;
		static EVP_PKEY* privateKey;
		static SSL_CTX* sslCtx;
		typedef std::map<RTC::FingerprintHash, std::string> Fingerprints;
		static Fingerprints fingerprints;
		typedef std::vector<SrtpProfileMapEntry> SRTPProfiles;
		static SRTPProfiles srtpProfiles;
		static MS_BYTE sslReadBuffer[];

	public:
		DTLSAgent(Listener* listener);
		virtual ~DTLSAgent();

		void Run(RTC::DTLSRole role);
		void SetRemoteFingerprint(RTC::FingerprintHash hash, std::string& fingerprint);
		void Reset();
		void Close();
		void ProcessDTLSData(const MS_BYTE* data, size_t len);
		bool IsRunning();
		bool IsConnected();
		void SendApplicationData(const MS_BYTE* data, size_t len);
		void Dump();

	private:
		bool CheckStatus(int return_code);
		void SendPendingOutgoingDTLSData();
		void SetTimeout();
		void ProcessHandshake();
		bool CheckRemoteFingerprint();
		RTC::SRTPProfile GetNegotiatedSRTPProfile();
		void ExtractSRTPKeys(RTC::SRTPProfile srtp_profile);

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
		RTC::DTLSRole role;
		RTC::FingerprintHash remoteFingerprintHash = RTC::FingerprintHash::NONE;
		std::string remoteFingerprint;
		bool isRunning = false;
		bool isHandshakeDone = false;
		bool isHandshakeDoneNow = false;
		bool isConnected = false;
		bool isCheckingStatus = false;
		bool doReset = false;
		bool doClose = false;
	};

	/* Inline static methods. */

	inline
	bool DTLSAgent::IsDTLS(const MS_BYTE* data, size_t len)
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
	bool DTLSAgent::IsConnected()
	{
		return this->isConnected;
	}

	inline
	bool DTLSAgent::IsRunning()
	{
		return this->isRunning;
	}
}

#endif
