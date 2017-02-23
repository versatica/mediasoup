#ifndef MS_RTC_TRANSPORT_HPP
#define MS_RTC_TRANSPORT_HPP

#include "common.hpp"
#include "RTC/UdpSocket.hpp"
#include "RTC/TcpServer.hpp"
#include "RTC/TcpConnection.hpp"
#include "RTC/IceCandidate.hpp"
#include "RTC/IceServer.hpp"
#include "RTC/StunMessage.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/DtlsTransport.hpp"
#include "RTC/SrtpSession.hpp"
#include "RTC/RtpListener.hpp"
#include "RTC/RtpReceiver.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "Channel/Request.hpp"
#include "Channel/Notifier.hpp"
#include <string>
#include <vector>
#include <json/json.h>

namespace RTC
{
	class Transport :
		public RTC::UdpSocket::Listener,
		public RTC::TcpServer::Listener,
		public RTC::TcpConnection::Listener,
		public RTC::IceServer::Listener,
		public RTC::DtlsTransport::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onTransportClosed(RTC::Transport* transport) = 0;
			virtual void onTransportRtcpPacket(RTC::Transport* transport, RTC::RTCP::Packet* packet) = 0;
		};

	public:
		Transport(Listener* listener, Channel::Notifier* notifier, uint32_t transportId, Json::Value& data);
		virtual ~Transport();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		void AddRtpReceiver(RTC::RtpReceiver* rtpReceiver);
		void RemoveRtpReceiver(RTC::RtpReceiver* rtpReceiver);
		void SendRtpPacket(RTC::RtpPacket* packet);
		void SendRtcpPacket(RTC::RTCP::Packet* packet);
		void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet);
		RTC::RtpReceiver* GetRtpReceiver(uint32_t ssrc);

	private:
		void MayRunDtlsTransport();

	/* Private methods to unify UDP and TCP behavior. */
	private:
		void onPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onStunDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onDtlsDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);
		void onRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len);

	/* Pure virtual methods inherited from RTC::UdpSocket::Listener. */
	public:
		virtual void onPacketRecv(RTC::UdpSocket *socket, const uint8_t* data, size_t len, const struct sockaddr* remote_addr) override;

	/* Pure virtual methods inherited from RTC::TcpServer::Listener. */
	public:
		virtual void onRtcTcpConnectionClosed(RTC::TcpServer* tcpServer, RTC::TcpConnection* connection, bool is_closed_by_peer) override;

	/* Pure virtual methods inherited from RTC::TcpConnection::Listener. */
	public:
		virtual void onPacketRecv(RTC::TcpConnection *connection, const uint8_t* data, size_t len) override;

	/* Pure virtual methods inherited from RTC::IceServer::Listener. */
	public:
		virtual void onOutgoingStunMessage(RTC::IceServer* iceServer, RTC::StunMessage* msg, RTC::TransportTuple* tuple) override;
		virtual void onIceSelectedTuple(IceServer* iceServer, RTC::TransportTuple* tuple) override;
		virtual void onIceConnected(IceServer* iceServer) override;
		virtual void onIceCompleted(IceServer* iceServer) override;
		virtual void onIceDisconnected(IceServer* iceServer) override;

	/* Pure virtual methods inherited from RTC::DtlsTransport::Listener. */
	public:
		virtual void onDtlsConnecting(DtlsTransport* dtlsTransport) override;
		virtual void onDtlsConnected(DtlsTransport* dtlsTransport, RTC::SrtpSession::Profile srtp_profile, uint8_t* srtp_local_key, size_t srtp_local_key_len, uint8_t* srtp_remote_key, size_t srtp_remote_key_len, std::string& remoteCert) override;
		virtual void onDtlsFailed(DtlsTransport* dtlsTransport) override;
		virtual void onDtlsClosed(DtlsTransport* dtlsTransport) override;
		virtual void onOutgoingDtlsData(RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;
		virtual void onDtlsApplicationData(RTC::DtlsTransport* dtlsTransport, const uint8_t* data, size_t len) override;

	public:
		// Passed by argument.
		uint32_t transportId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
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
		bool remoteDtlsParametersGiven = false;
		RTC::DtlsTransport::Role dtlsLocalRole = RTC::DtlsTransport::Role::AUTO;
		// Others (RtpListener).
		RtpListener rtpListener;
	};

	/* Inline instance methods. */

	inline
	void Transport::AddRtpReceiver(RTC::RtpReceiver* rtpReceiver)
	{
		this->rtpListener.AddRtpReceiver(rtpReceiver);
	}

	inline
	void Transport::RemoveRtpReceiver(RTC::RtpReceiver* rtpReceiver)
	{
		this->rtpListener.RemoveRtpReceiver(rtpReceiver);
	}

	inline
	RTC::RtpReceiver* Transport::GetRtpReceiver(uint32_t ssrc)
	{
		return this->rtpListener.GetRtpReceiver(ssrc);
	}
}

#endif
