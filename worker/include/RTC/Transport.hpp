#ifndef MS_RTC_TRANSPORT_HPP
#define MS_RTC_TRANSPORT_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/DtlsTransport.hpp"
#include "RTC/IceCandidate.hpp"
#include "RTC/IceServer.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RemoteBitrateEstimator/RemoteBitrateEstimatorAbsSendTime.hpp"
#include "RTC/RtpListener.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpReceiver.hpp"
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
			virtual void onTransportConnected(RTC::Transport* transport)                             = 0;
			virtual void onTransportClosed(RTC::Transport* transport)                                = 0;
			virtual void onTransportRtcpPacket(RTC::Transport* transport, RTC::RTCP::Packet* packet) = 0;
			virtual void onTransportFullFrameRequired(RTC::Transport* transport)                     = 0;
		};

	public:
		Transport(Listener* listener, Channel::Notifier* notifier, uint32_t transportId, Json::Value& data);

	private:
		virtual ~Transport();

	public:
		void Destroy();
		Json::Value toJson() const;
		void HandleRequest(Channel::Request* request);
		void AddRtpReceiver(RTC::RtpReceiver* rtpReceiver);
		void RemoveRtpReceiver(const RTC::RtpReceiver* rtpReceiver);
		void SendRtpPacket(RTC::RtpPacket* packet);
		void SendRtcpPacket(RTC::RTCP::Packet* packet);
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet);
		RTC::RtpReceiver* GetRtpReceiver(uint32_t ssrc);
		bool IsConnected() const;
		void EnableRemb();
		bool HasRemb();

	private:
		void MayRunDtlsTransport();

		/* Private methods to unify UDP and TCP behavior. */
	private:
		void onPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onStunDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onDtlsDataRecv(const RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

		/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		virtual void onPacketRecv(
		    RTC::UdpSocket* socket,
		    const uint8_t* data,
		    size_t len,
		    const struct sockaddr* remoteAddr) override;

		/* Pure virtual methods inherited from RTC::TcpServer::Listener. */
	public:
		virtual void onRtcTcpConnectionClosed(
		    RTC::TcpServer* tcpServer, RTC::TcpConnection* connection, bool isClosedByPeer) override;

		/* Pure virtual methods inherited from RTC::TcpConnection::Listener. */
	public:
		virtual void onPacketRecv(RTC::TcpConnection* connection, const uint8_t* data, size_t len) override;

		/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		virtual void onOutgoingStunMessage(
		    const RTC::IceServer* iceServer,
		    const RTC::StunMessage* msg,
		    RTC::TransportTuple* tuple) override;
		virtual void onIceSelectedTuple(const RTC::IceServer* iceServer, RTC::TransportTuple* tuple) override;
		virtual void onIceConnected(const RTC::IceServer* iceServer) override;
		virtual void onIceCompleted(const RTC::IceServer* iceServer) override;
		virtual void onIceDisconnected(const RTC::IceServer* iceServer) override;

		/* Pure virtual methods inherited from RTC::DtlsTransport::Listener. */
	public:
		virtual void onDtlsConnecting(const RTC::DtlsTransport* dtlsTransport) override;
		virtual void onDtlsConnected(
		    const RTC::DtlsTransport* dtlsTransport,
		    RTC::SrtpSession::Profile srtpProfile,
		    uint8_t* srtpLocalKey,
		    size_t srtpLocalKeyLen,
		    uint8_t* srtpRemoteKey,
		    size_t srtpRemoteKeyLen,
		    std::string& remoteCert) override;
		virtual void onDtlsFailed(const RTC::DtlsTransport* dtlsTransport) override;
		virtual void onDtlsClosed(const RTC::DtlsTransport* dtlsTransport) override;
		virtual void onOutgoingDtlsData(
		    const RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;
		virtual void onDtlsApplicationData(
		    const RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;

		/* Pure virtual methods inherited from RTC::RemoteBitrateEstimator::Listener. */
	public:
		virtual void onReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs, uint32_t bitrate) override;

	public:
		// Passed by argument.
		uint32_t transportId;

	private:
		// Passed by argument.
		Listener* listener          = nullptr;
		Channel::Notifier* notifier = nullptr;
		// Allocated by this.
		RTC::IceServer* iceServer = nullptr;
		std::vector<RTC::UdpSocket*> udpSockets;
		std::vector<RTC::TcpServer*> tcpServers;
		RTC::DtlsTransport* dtlsTransport = nullptr;
		RTC::SrtpSession* srtpRecvSession = nullptr;
		RTC::SrtpSession* srtpSendSession = nullptr;
		// Others.
		bool allocated = false;
		// Others (ICE).
		std::vector<IceCandidate> iceLocalCandidates;
		RTC::TransportTuple* selectedTuple = nullptr;
		// Others (DTLS).
		bool remoteDtlsParametersGiven         = false;
		RTC::DtlsTransport::Role dtlsLocalRole = RTC::DtlsTransport::Role::AUTO;
		// Others (RtpListener).
		RtpListener rtpListener;
		// REMB and bitrate stuff.
		std::unique_ptr<RTC::RemoteBitrateEstimatorAbsSendTime> remoteBitrateEstimator;
		uint32_t maxBitrate                = 0;
		uint32_t effectiveMaxBitrate       = 0;
		uint64_t lastEffectiveMaxBitrateAt = 0;
	};

	/* Inline instance methods. */

	inline void Transport::AddRtpReceiver(RTC::RtpReceiver* rtpReceiver)
	{
		this->rtpListener.AddRtpReceiver(rtpReceiver);
	}

	inline void Transport::RemoveRtpReceiver(const RTC::RtpReceiver* rtpReceiver)
	{
		this->rtpListener.RemoveRtpReceiver(rtpReceiver);
	}

	inline RTC::RtpReceiver* Transport::GetRtpReceiver(uint32_t ssrc)
	{
		return this->rtpListener.GetRtpReceiver(ssrc);
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
}

#endif
