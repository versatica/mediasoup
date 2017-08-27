#ifndef MS_RTC_TRANSPORT_HPP
#define MS_RTC_TRANSPORT_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/DtlsTransport.hpp"
#include "RTC/IceCandidate.hpp"
#include "RTC/IceServer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "RTC/RtpListener.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SrtpSession.hpp"
#include "RTC/StunMessage.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include <json/json.h>
#include <string>
#include <vector>

namespace RTC
{
	class Transport : public RTC::UdpSocket::Listener,
	                  public RTC::TcpServer::Listener,
	                  public RTC::TcpConnection::Listener,
	                  public RTC::IceServer::Listener,
	                  public RTC::DtlsTransport::Listener,
	                  public RTC::RemoteBitrateEstimator::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnTransportConnected(RTC::Transport* transport)                             = 0;
			virtual void OnTransportClosed(RTC::Transport* transport)                                = 0;
			virtual void OnTransportRtcpPacket(RTC::Transport* transport, RTC::RTCP::Packet* packet) = 0;
			virtual void OnTransportFullFrameRequired(RTC::Transport* transport)                     = 0;
		};

	public:
		Transport(Listener* listener, Channel::Notifier* notifier, uint32_t transportId, Json::Value& data);

	private:
		~Transport() override;

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		void AddProducer(RTC::Producer* producer);
		void RemoveProducer(const RTC::Producer* producer);
		void SendRtpPacket(RTC::RtpPacket* packet);
		void SendRtcpPacket(RTC::RTCP::Packet* packet);
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet);
		RTC::Producer* GetProducer(uint32_t ssrc);
		bool IsConnected() const;
		void EnableRemb();
		bool HasRemb();

	private:
		void MayRunDtlsTransport();

		/* Private methods to unify UDP and TCP behavior. */
	private:
		void OnPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnStunDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnDtlsDataRecv(const RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void OnRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		void OnPacketRecv(
		    RTC::UdpSocket* socket,
		    const uint8_t* data,
		    size_t len,
		    const struct sockaddr* remoteAddr) override;

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
		    const RTC::IceServer* iceServer,
		    const RTC::StunMessage* msg,
		    RTC::TransportTuple* tuple) override;
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

		/* Pure virtual methods inherited from RTC::RemoteBitrateEstimator::Listener. */
	public:
		void OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs, uint32_t bitrate) override;

	public:
		// Passed by argument.
		uint32_t transportId{ 0 };

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		Channel::Notifier* notifier{ nullptr };
		// Allocated by this.
		RTC::IceServer* iceServer{ nullptr };
		std::vector<RTC::UdpSocket*> udpSockets;
		std::vector<RTC::TcpServer*> tcpServers;
		RTC::DtlsTransport* dtlsTransport{ nullptr };
		RTC::SrtpSession* srtpRecvSession{ nullptr };
		RTC::SrtpSession* srtpSendSession{ nullptr };
		// Others.
		bool allocated{ false };
		// Others (ICE).
		std::vector<IceCandidate> iceLocalCandidates;
		RTC::TransportTuple* selectedTuple{ nullptr };
		// Others (DTLS).
		bool hasRemoteDtlsParameters{ false };
		RTC::DtlsTransport::Role dtlsLocalRole{ RTC::DtlsTransport::Role::AUTO };
		// Others (RtpListener).
		RtpListener rtpListener;
		// REMB and bitrate stuff.
		std::unique_ptr<RTC::RemoteBitrateEstimatorAbsSendTime> remoteBitrateEstimator;
		uint32_t maxBitrate{ 0 };
		uint32_t effectiveMaxBitrate{ 0 };
		uint64_t lastEffectiveMaxBitrateAt{ 0 };
	};

	/* Inline instance methods. */

	inline void Transport::AddProducer(RTC::Producer* producer)
	{
		this->rtpListener.AddProducer(producer);
	}

	inline void Transport::RemoveProducer(const RTC::Producer* producer)
	{
		this->rtpListener.RemoveProducer(producer);
	}

	inline RTC::Producer* Transport::GetProducer(uint32_t ssrc)
	{
		return this->rtpListener.GetProducer(ssrc);
	}

	inline bool Transport::IsConnected() const
	{
		return this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTED;
	}

	inline void Transport::EnableRemb()
	{
		if (!this->remoteBitrateEstimator)
		{
			this->remoteBitrateEstimator.reset(new RTC::RemoteBitrateEstimatorAbsSendTime(this));
		}
	}

	inline bool Transport::HasRemb()
	{
		if (this->remoteBitrateEstimator)
			return true;
		else
			return false;
	}
} // namespace RTC

#endif
