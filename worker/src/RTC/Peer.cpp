#define MS_CLASS "RTC::Peer"

#include "RTC/Peer.h"
#include "RTC/DTLSRole.h"
#include "MediaSoupError.h"
#include "Logger.h"
// TMP
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include <string>

namespace RTC
{
	/* Instance methods. */

	Peer::Peer(Listener* listener) :
		listener(listener)
	{
		MS_TRACE();

		// TMP

		this->transport = RTC::Transport::NewWebRTC(this);

		// Local DTLS role.
		RTC::DTLSRole local_dtls_role = RTC::DTLSRole::SERVER;

		// Chrome's fingerprint.
		std::string remote_fingerprint = "74:24:E1:0A:30:48:FB:F2:72:6E:2C:70:FA:96:FD:06:81:AA:F8:1F:65:2E:D4:81:3E:93:0C:0F:E1:D0:FE:19";

		this->transport->SetLocalDTLSRole(local_dtls_role);

		this->transport->SetRemoteDTLSFingerprint(RTC::FingerprintHash::SHA256, remote_fingerprint);

		try
		{
			RTC::UDPSocket* socket = RTC::UDPSocket::New(AF_INET);
			this->transport->AddUDPSocket(socket);
			MS_WARN("---- TEST: transport listens in IPv4 UDP '%s' : %u", socket->GetLocalIP().c_str(), socket->GetLocalPort());
		}
		catch (const MediaSoupError &error)
		{
			MS_ERROR("---- TEST: error adding IPv4 UDP to transport: %s", error.what());
		}

		try
		{
			RTC::TCPServer* server = RTC::TCPServer::New(AF_INET);
			this->transport->AddTCPServer(server);
			MS_WARN("---- TEST: transport listens in IPv4 TCP '%s' : %u", server->GetLocalIP().c_str(), server->GetLocalPort());
		}
		catch (const MediaSoupError &error)
		{
			MS_ERROR("---- TEST: error adding IPv4 TCP to transport: %s", error.what());
		}
	}

	Peer::~Peer()
	{
		MS_TRACE();
	}

	void Peer::SendRTPPacket(RTC::RTPPacket* packet)
	{
		MS_TRACE();

		// TMP

		if (this->transport->IsReadyForMedia())
			this->transport->SendRTPPacket(packet);
	}

	void Peer::SendRTCPPacket(RTC::RTCPPacket* packet)
	{
		MS_TRACE();

		// TMP

		if (this->transport->IsReadyForMedia())
			this->transport->SendRTCPPacket(packet);
	}

	void Peer::Close()
	{
		MS_TRACE();

		// TMP
		if (this->transport)
			this->transport->Close();

		delete this;
	}

	void Peer::onRTPPacket(RTC::Transport* transport, RTC::RTPPacket* packet)
	{
		MS_TRACE();

		// MS_DEBUG("RTP packet received");  // TODO: TMP

		// TMP

		this->listener->onRTPPacket(this, packet);
	}

	void Peer::onRTCPPacket(RTC::Transport* transport, RTC::RTCPPacket* packet)
	{
		MS_TRACE();

		// MS_DEBUG("RTCP packet received");  // TODO: TMP

		// TMP

		this->listener->onRTCPPacket(this, packet);
	}
}
