#ifndef MS_RTC_PLAIN_TRANSPORT_HPP
#define MS_RTC_PLAIN_TRANSPORT_HPP

#include "RTC/SrtpSession.hpp"
#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include <absl/container/flat_hash_map.h>

namespace RTC
{
	class PlainTransport : public RTC::Transport, public RTC::UdpSocket::Listener
	{
	private:
		struct ListenIp
		{
			std::string ip;
			std::string announcedIp;
		};

	private:
		static absl::flat_hash_map<std::string, RTC::SrtpSession::CryptoSuite> string2SrtpCryptoSuite;
		static absl::flat_hash_map<RTC::SrtpSession::CryptoSuite, std::string> srtpCryptoSuite2String;
		static size_t srtpMasterLength;

	public:
		PlainTransport(const std::string& id, RTC::Transport::Listener* listener, json& data);
		~PlainTransport() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) override;
		void HandleRequest(Channel::ChannelRequest* request) override;
		void HandleNotification(PayloadChannel::Notification* notification) override;

	private:
		bool IsConnected() const override;
		bool HasSrtp() const;
		bool IsSrtpReady() const;
		void SendRtpPacket(
		  RTC::Consumer* consumer,
		  RTC::RtpPacket* packet,
		  RTC::Transport::onSendCallback* cb = nullptr) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;
		void SendMessage(
		  RTC::DataConsumer* dataConsumer,
		  uint32_t ppid,
		  const uint8_t* msg,
		  size_t len,
		  onQueuedCallback* cb = nullptr) override;
		void SendSctpData(const uint8_t* data, size_t len) override;
		void RecvStreamClosed(uint32_t ssrc) override;
		void SendStreamClosed(uint32_t ssrc) override;
		void OnPacketReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtcpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnSctpDataReceived(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		void OnUdpSocketPacketReceived(
		  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) override;

	private:
		// Allocated by this.
		RTC::UdpSocket* udpSocket{ nullptr };
		RTC::UdpSocket* rtcpUdpSocket{ nullptr };
		RTC::TransportTuple* tuple{ nullptr };
		RTC::TransportTuple* rtcpTuple{ nullptr };
		RTC::SrtpSession* srtpRecvSession{ nullptr };
		RTC::SrtpSession* srtpSendSession{ nullptr };
		// Others.
		ListenIp listenIp;
		bool rtcpMux{ true };
		bool comedia{ false };
		struct sockaddr_storage remoteAddrStorage;
		struct sockaddr_storage rtcpRemoteAddrStorage;
		RTC::SrtpSession::CryptoSuite srtpCryptoSuite{
			RTC::SrtpSession::CryptoSuite::AES_CM_128_HMAC_SHA1_80
		};
		std::string srtpKey;
		std::string srtpKeyBase64;
		bool connectCalled{ false }; // Whether connect() was succesfully called.
	};
} // namespace RTC

#endif
