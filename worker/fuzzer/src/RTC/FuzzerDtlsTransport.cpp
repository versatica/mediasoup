#define MS_CLASS "FuzzerRtcDtlsTransport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/FuzzerDtlsTransport.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace
{
	// NOLINTBEGIN(readability-identifier-naming)
	// DtlsTransport instance. It's reset every time DTLS handshake fails or DTLS
	// is closed.
	thread_local RTC::DtlsTransport* dtlsTransportSingleton{ nullptr };
	// DtlsTransport Listener instance. It's reset every time the DtlsTransport
	// singletonDTLS is reset.
	thread_local FuzzerRtcDtlsTransport::DtlsTransportListener* dtlsTransportListenerSingleton{ nullptr };
	// NOLINTEND(readability-identifier-naming)
} // namespace

void FuzzerRtcDtlsTransport::Fuzz(const uint8_t* data, size_t len)
{
	if (!RTC::DtlsTransport::IsDtls(data, len))
	{
		return;
	}

	if (!dtlsTransportSingleton)
	{
		MS_DEBUG_DEV("no DtlsTransport singleton, creating it");

		delete dtlsTransportListenerSingleton;
		dtlsTransportListenerSingleton = new DtlsTransportListener();

		dtlsTransportSingleton = new RTC::DtlsTransport(dtlsTransportListenerSingleton);

		RTC::DtlsTransport::Role localRole;
		RTC::DtlsTransport::Fingerprint dtlsRemoteFingerprint;

		// Local DTLS role must be 'server' or 'client'. Choose it based on
		// randomness of first given byte.
		if (data[0] / 2 == 0)
		{
			localRole = RTC::DtlsTransport::Role::SERVER;
		}
		else
		{
			localRole = RTC::DtlsTransport::Role::CLIENT;
		}

		// Remote DTLS fingerprint random generation.
		// NOTE: Use a random integer in range 1..5 since FingerprintAlgorithm enum
		// has 5 possible values starting with value 1.
		dtlsRemoteFingerprint.algorithm = static_cast<RTC::DtlsTransport::FingerprintAlgorithm>(
		  Utils::Crypto::GetRandomUInt<uint16_t>(1u, 5u));

		dtlsRemoteFingerprint.value =
		  Utils::Crypto::GetRandomString(Utils::Crypto::GetRandomUInt<uint16_t>(3u, 20u));

		dtlsTransportSingleton->Run(localRole);
		dtlsTransportSingleton->SetRemoteFingerprint(dtlsRemoteFingerprint);
	}

	dtlsTransportSingleton->ProcessDtlsData(data, len);

	// DTLS may have failed or closed after ProcessDtlsData(). If so, unset it.
	if (
	  dtlsTransportSingleton->GetState() == RTC::DtlsTransport::DtlsState::FAILED ||
	  dtlsTransportSingleton->GetState() == RTC::DtlsTransport::DtlsState::CLOSED)
	{
		MS_DEBUG_DEV("DtlsTransport singleton state is 'failed' or 'closed', unsetting it");

		delete dtlsTransportSingleton;
		dtlsTransportSingleton = nullptr;
	}
	else
	{
		dtlsTransportSingleton->SendApplicationData(data, len);
	}
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportConnecting(
  const RTC::DtlsTransport* /*dtlsTransport*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton connecting");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportConnected(
  const RTC::DtlsTransport* /*dtlsTransport*/,
  RTC::SrtpSession::CryptoSuite /*srtpCryptoSuite*/,
  uint8_t* /*srtpLocalKey*/,
  size_t /*srtpLocalKeyLen*/,
  uint8_t* /*srtpRemoteKey*/,
  size_t /*srtpRemoteKeyLen*/,
  std::string& /*remoteCert*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton connected");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportFailed(
  const RTC::DtlsTransport* /*dtlsTransport*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton failed");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportClosed(
  const RTC::DtlsTransport* /*dtlsTransport*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton closed");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportSendData(
  const RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* /*data*/, size_t /*len*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton wants to send data");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportApplicationDataReceived(
  const RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* /*data*/, size_t /*len*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton received application data");
}
