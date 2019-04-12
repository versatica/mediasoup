#ifndef MS_RTC_WEBRTC_TRANSPORT_HPP
#define MS_RTC_WEBRTC_TRANSPORT_HPP

#include "RTC/DtlsTransport.hpp"
#include "RTC/IceCandidate.hpp"
#include "RTC/IceServer.hpp"
#include "RTC/RembClient.hpp"
#include "RTC/RembServer/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "RTC/SrtpSession.hpp"
#include "RTC/StunMessage.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/Transport.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include <vector>

namespace RTC
{
	class WebRtcTransport : public RTC::Transport,
	                        public RTC::UdpSocket::Listener,
	                        public RTC::TcpServer::Listener,
	                        public RTC::TcpConnection::Listener,
	                        public RTC::IceServer::Listener,
	                        public RTC::DtlsTransport::Listener,
	                        public RTC::RembClient::Listener,
	                        public RTC::RembServer::RemoteBitrateEstimator::Listener
	{
	private:
		struct ListenIp
		{
			std::string ip;
			std::string announcedIp;
		};

	public:
		WebRtcTransport(const std::string& id, RTC::Transport::Listener* listener, json& data);
		~WebRtcTransport() override;

	public:
		void FillJson(json& jsonObject) const override;
		void FillJsonStats(json& jsonArray) const override;
		void HandleRequest(Channel::Request* request) override;

	private:
		bool IsConnected() const override;
		void MayRunDtlsTransport();
		void SendRtpPacket(RTC::RtpPacket* packet, RTC::Consumer* consumer) override;
		void SendRtcpPacket(RTC::RTCP::Packet* packet) override;
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) override;
		void OnPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnStunDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnDtlsDataRecv(const RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::Transport. */
	private:
		void UserOnNewProducer(RTC::Producer* producer) override;
		void UserOnNewConsumer(RTC::Consumer* consumer) override;
		void UserOnRembFeedback(RTC::RTCP::FeedbackPsRembPacket* remb) override;

		/* Pure virtual methods inherited from RTC::Consumer::Listener. */
	public:
		void OnConsumerNeedBitrateChange(RTC::Consumer* consumer) override;

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		void OnPacketRecv(
		  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) override;

		/* Pure virtual methods inherited from RTC::TcpServer::Listener. */
	public:
		void OnRtcTcpConnectionClosed(
		  RTC::TcpServer* tcpServer, RTC::TcpConnection* connection, bool isClosedByPeer) override;

		/* Pure virtual methods inherited from RTC::TcpConnection::Listener. */
	public:
		void OnPacketRecv(RTC::TcpConnection* connection, const uint8_t* data, size_t len) override;

		/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		void OnOutgoingStunMessage(
		  const RTC::IceServer* iceServer, const RTC::StunMessage* msg, RTC::TransportTuple* tuple) override;
		void OnIceSelectedTuple(const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) override;
		void OnIceConnected(const RTC::IceServer* iceServer) override;
		void OnIceCompleted(const RTC::IceServer* iceServer) override;
		void OnIceDisconnected(const RTC::IceServer* iceServer) override;

		/* Pure virtual methods inherited from RTC::DtlsTransport::Listener. */
	public:
		void OnDtlsConnecting(const RTC::DtlsTransport* dtlsTransport) override;
		void OnDtlsConnected(
		  const RTC::DtlsTransport* dtlsTransport,
		  RTC::SrtpSession::Profile srtpProfile,
		  uint8_t* srtpLocalKey,
		  size_t srtpLocalKeyLen,
		  uint8_t* srtpRemoteKey,
		  size_t srtpRemoteKeyLen,
		  std::string& remoteCert) override;
		void OnDtlsFailed(const RTC::DtlsTransport* dtlsTransport) override;
		void OnDtlsClosed(const RTC::DtlsTransport* dtlsTransport) override;
		void OnOutgoingDtlsData(
		  const RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;
		void OnDtlsApplicationData(
		  const RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;

		/* Pure virtual methods inherited from RTC::RembClient::Listener. */
	public:
		void OnRembClientIncreaseBitrate(RTC::RembClient* rembClient, uint32_t bitrate) override;
		void OnRembClientDecreaseBitrate(RTC::RembClient* rembClient, uint32_t bitrate) override;

		/* Pure virtual methods inherited from RTC::RembServer::RemoteBitrateEstimator::Listener. */
	public:
		void OnRembServerAvailableBitrate(
		  const RTC::RembServer::RemoteBitrateEstimator* remoteBitrateEstimator,
		  const std::vector<uint32_t>& ssrcs,
		  uint32_t availableBitrate) override;

	private:
		// Allocated by this.
		RTC::IceServer* iceServer{ nullptr };
		// Map of UdpSocket/TcpServer and local announced IP (if any).
		std::unordered_map<RTC::UdpSocket*, std::string> udpSockets;
		std::unordered_map<RTC::TcpServer*, std::string> tcpServers;
		RTC::DtlsTransport* dtlsTransport{ nullptr };
		RTC::SrtpSession* srtpRecvSession{ nullptr };
		RTC::SrtpSession* srtpSendSession{ nullptr };
		RTC::RembClient* rembClient{ nullptr };
		RTC::RembServer::RemoteBitrateEstimatorAbsSendTime* rembServer{ nullptr };
		// Others.
		bool connected{ false }; // Whether connect() was succesfully called.
		std::vector<RTC::IceCandidate> iceCandidates;
		RTC::TransportTuple* iceSelectedTuple{ nullptr };
		RTC::DtlsTransport::Role dtlsRole{ RTC::DtlsTransport::Role::AUTO };
		uint32_t maxIncomingBitrate{ 0 };
	};
} // namespace RTC

#endif
