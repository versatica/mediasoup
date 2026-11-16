#define MS_CLASS "FuzzerRtcDtlsTransport"
// #define MS_LOG_DEV_LEVEL 3

#include "fuzzer/include/RTC/FuzzerDtlsTransport.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "mocks/include/MockShared.hpp"

void FuzzerRtcDtlsTransport::Fuzz(const uint8_t* data, size_t len)
{
	// NOTE: These are destroyed in reverse order, so the DtlsTransport goes
	// before the listener and the shared instance it points to.
	static thread_local mocks::MockShared shared(/*getTimeUs*/
	                                             []() -> int64_t
	                                             {
		                                             return 1000 * 1000;
	                                             });
	static thread_local DtlsTransportListener dtlsTransportListener;

	// DtlsTransport instance. It's reset every time DTLS handshake fails or DTLS
	// is closed.
	static thread_local std::unique_ptr<RTC::DtlsTransport> dtlsTransport;

	if (!RTC::DtlsTransport::IsDtls(data, len))
	{
		return;
	}

	if (!dtlsTransport)
	{
		MS_DEBUG_DEV("no DtlsTransport, creating it");

		dtlsTransport = std::make_unique<RTC::DtlsTransport>(
		  std::addressof(dtlsTransportListener), std::addressof(shared));

		// Local DTLS role must be 'server' or 'client'. Choose it based on
		// randomness of first given byte.
		const auto localRole =
		  data[0] % 2 == 0 ? RTC::DtlsTransport::Role::SERVER : RTC::DtlsTransport::Role::CLIENT;

		// Remote DTLS fingerprint random generation.
		// NOTE: Use a random integer in range 1..5 since FingerprintAlgorithm enum
		// has 5 possible values starting with value 1.
		const RTC::DtlsTransport::Fingerprint dtlsRemoteFingerprint{
			.algorithm = static_cast<RTC::DtlsTransport::FingerprintAlgorithm>(
			  Utils::Crypto::GetRandomUInt<uint16_t>(1, 5)),
			.value = Utils::Crypto::GetRandomString(Utils::Crypto::GetRandomUInt<uint16_t>(3, 20)),
		};

		dtlsTransport->Run(localRole);
		dtlsTransport->SetRemoteFingerprint(dtlsRemoteFingerprint);
	}

	dtlsTransport->ProcessDtlsData(data, len, shared.GetTimeUs());

	// DTLS may have failed or closed after ProcessDtlsData(). If so, unset it.
	if (
	  dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::FAILED ||
	  dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CLOSED)
	{
		MS_DEBUG_DEV("DtlsTransport state is 'failed' or 'closed', unsetting it");

		dtlsTransport.reset();
	}
	else
	{
		dtlsTransport->SendApplicationData(data, len);
	}
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportConnecting(
  RTC::DtlsTransport* /*dtlsTransport*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton connecting");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportConnected(
  RTC::DtlsTransport* /*dtlsTransport*/,
  RTC::SrtpSession::CryptoSuite /*srtpCryptoSuite*/,
  const uint8_t* /*srtpLocalKey*/,
  size_t /*srtpLocalKeyLen*/,
  const uint8_t* /*srtpRemoteKey*/,
  size_t /*srtpRemoteKeyLen*/,
  std::string& /*remoteCert*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton connected");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportFailed(
  RTC::DtlsTransport* /*dtlsTransport*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton failed");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportClosed(
  RTC::DtlsTransport* /*dtlsTransport*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton closed");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportSendData(
  RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* /*data*/, size_t /*len*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton wants to send data");
}

void FuzzerRtcDtlsTransport::DtlsTransportListener::OnDtlsTransportApplicationDataReceived(
  RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* /*data*/, size_t /*len*/, int64_t /*receivedAtUs*/)
{
	MS_DEBUG_DEV("DtlsTransport singleton received application data");
}
