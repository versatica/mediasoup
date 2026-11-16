#ifndef MS_FUZZER_RTC_DTLS_TRANSPORT_HPP
#define MS_FUZZER_RTC_DTLS_TRANSPORT_HPP

#include "common.hpp"
#include "RTC/DtlsTransport.hpp"

namespace FuzzerRtcDtlsTransport
{
	class DtlsTransportListener : public RTC::DtlsTransport::Listener
	{
		/* Pure virtual methods inherited from RTC::DtlsTransport::Listener. */
	public:
		void OnDtlsTransportConnecting(RTC::DtlsTransport* dtlsTransport) override;
		void OnDtlsTransportConnected(
		  RTC::DtlsTransport* dtlsTransport,
		  RTC::SrtpSession::CryptoSuite srtpCryptoSuite,
		  const uint8_t* srtpLocalKey,
		  size_t srtpLocalKeyLen,
		  const uint8_t* srtpRemoteKey,
		  size_t srtpRemoteKeyLen,
		  std::string& remoteCert) override;
		void OnDtlsTransportFailed(RTC::DtlsTransport* dtlsTransport) override;
		void OnDtlsTransportClosed(RTC::DtlsTransport* dtlsTransport) override;
		void OnDtlsTransportSendData(
		  RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;
		void OnDtlsTransportApplicationDataReceived(
		  RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len, int64_t receivedAtUs) override;
	};

	void Fuzz(const uint8_t* data, size_t len);
} // namespace FuzzerRtcDtlsTransport

#endif
