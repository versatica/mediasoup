#define MS_CLASS "RTC::Transport"

#include "RTC/Transport.h"
#include "RTC/FingerprintHash.h"
#include "MediaSoupError.h"
#include "Logger.h"

#define TRANSPORT_UDP  "UDP"
#define TRANSPORT_TCP  "TCP"
#define TRANSPORT_NAME(source)  (source->IsUDP() ? TRANSPORT_UDP : TRANSPORT_TCP)

namespace RTC
{
	/* Static variables. */

	size_t Transport::maxSources = 8;

	/* Static methods. */

	Transport* Transport::NewWebRTC(Listener* listener)
	{
		MS_TRACE();

		return new Transport(listener, Flag::ICE | Flag::DTLS | Flag::SRTP);
	}

	/* Instance methods. */

	Transport::Transport(Listener* listener, int flags) :
		listener(listener),
		flags(flags)
	{
		MS_TRACE();

		if (HasFlagICE())
			this->iceServer = new RTC::ICEServer(this);

		if (HasFlagDTLS())
		{
			this->dtlsHandler = new RTC::DTLSHandler(this);

			// If DTLS flag is given then ensure SRTP flag is set.
			this->flags |= Flag::SRTP;
		}
	}

	Transport::~Transport()
	{
		MS_TRACE();
	}

	void Transport::AddUDPSocket(RTC::UDPSocket* socket)
	{
		MS_TRACE();

		socket->SetListener(this);
		this->udpSockets.push_back(socket);
	}

	void Transport::AddTCPServer(RTC::TCPServer* server)
	{
		MS_TRACE();

		server->SetListeners(this, this);
		this->tcpServers.push_back(server);
	}

	void Transport::Reset()
	{
		MS_TRACE();

		this->sendingSource = nullptr;
		for (auto it = this->sources.begin(); it != this->sources.end(); ++it)
		{
			RTC::TransportSource* source = &(*it);

			source->Close();
		}
		this->sources.clear();

		this->isIcePaired = false;
		this->isIcePairedWithUseCandidate = false;

		if (this->dtlsHandler && this->dtlsHandler->IsRunning())
			this->dtlsHandler->Reset();

		if (this->srtpRecvSession)
		{
			this->srtpRecvSession->Close();
			this->srtpRecvSession = nullptr;
		}

		if (this->srtpSendSession)
		{
			this->srtpSendSession->Close();
			this->srtpSendSession = nullptr;
		}
	}

	void Transport::Close()
	{
		MS_TRACE();

		if (this->iceServer)
			this->iceServer->Close();

		if (this->dtlsHandler)
			this->dtlsHandler->Close();

		if (this->srtpRecvSession)
			this->srtpRecvSession->Close();

		if (this->srtpSendSession)
			this->srtpSendSession->Close();

		for (auto socket : this->udpSockets)
			socket->Close();
		for (auto server : this->tcpServers)
			server->Close();

		delete this;
	}

	void Transport::SetLocalDTLSRole(RTC::DTLSRole role)
	{
		MS_TRACE();

		if (!HasFlagDTLS())
		{
			MS_ERROR("no DTLS flag");
			return;
		}

		// If no changes are being made return.
		if (this->dtlsRole == role)
		{
			MS_DEBUG("given DTLS role matches the current one, doing nothing");
			return;
		}

		// Update local DTLS role.
		this->dtlsRole = role;

		// Otherwise, if the DTLS handler is running, reset it.
		if (this->dtlsHandler->IsRunning())
			this->dtlsHandler->Reset();

		// If ready, run the DTLS handler.
		if (this->dtlsRole != RTC::DTLSRole::NONE)
			RunDTLSHandlerIfReady();
	}

	void Transport::SetRemoteDTLSFingerprint(RTC::FingerprintHash hash, std::string& fingerprint)
	{
		MS_TRACE();

		if (!HasFlagDTLS())
		{
			MS_ERROR("no DTLS flag");
			return;
		}

		this->dtlsHandler->SetRemoteFingerprint(hash, fingerprint);
	}

	void Transport::SetLocalSRTPKey(RTC::SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len)
	{
		MS_TRACE();

		if (!HasFlagSRTP())
		{
			MS_ERROR("transport without SRTP");
			return;
		}

		// Close it if it was already set and update it.
		if (this->srtpSendSession)
		{
			this->srtpSendSession->Close();
			this->srtpSendSession = nullptr;
		}

		try
		{
			this->srtpSendSession = new RTC::SRTPSession(SRTPSession::Type::OUTBOUND, srtp_profile, srtp_local_key, srtp_local_key_len);
		}
		catch (const MediaSoupError &error)
		{
			MS_ERROR("error creating SRTP sending session: %s", error.what());
		}
	}

	void Transport::SetRemoteSRTPKey(RTC::SRTPProfile srtp_profile, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len)
	{
		MS_TRACE();

		if (!HasFlagSRTP())
		{
			MS_ERROR("transport without SRTP");
			return;
		}

		// Close it if it was already set and update it.
		if (this->srtpRecvSession)
		{
			this->srtpRecvSession->Close();
			this->srtpRecvSession = nullptr;
		}

		try
		{
			this->srtpRecvSession = new RTC::SRTPSession(SRTPSession::Type::INBOUND, srtp_profile, srtp_remote_key, srtp_remote_key_len);
		}
		catch (const MediaSoupError &error)
		{
			MS_ERROR("error creating SRTP receiving session: %s", error.what());
		}
	}

	void Transport::SendRTPPacket(RTC::RTPPacket* packet)
	{
		MS_TRACE();

		if (!this->sendingSource)
		{
			MS_WARN("no sending address set, cannot send RTP packet");
			return;
		}

		const MS_BYTE* data = packet->GetRaw();
		size_t len = packet->GetLength();

		if (HasFlagSRTP())
		{
			if (!this->srtpSendSession)
			{
				MS_WARN("SRTP sending session not set, cannot send RTP packet");
				return;
			}

			if (!this->srtpSendSession->EncryptRTP(&data, &len))
			{
				MS_WARN("error encrypting RTP packet: %s", this->srtpSendSession->GetLastErrorDesc());
				return;
			}

			MS_DEBUG("sending SRTP packet of %zu bytes", len);
		}
		else
		{
			MS_DEBUG("sending RTP packet of %zu bytes", len);
		}

		this->sendingSource->Send(data, len);
	}

	void Transport::SendRTCPPacket(RTC::RTCPPacket* packet)
	{
		MS_TRACE();

		if (!this->sendingSource)
		{
			MS_WARN("no sending address set, cannot send RTCP packet");
			return;
		}

		const MS_BYTE* data = packet->GetRaw();
		size_t len = packet->GetLength();

		if (HasFlagSRTP())
		{
			if (!this->srtpSendSession)
			{
				MS_WARN("SRTP sending session not set, cannot send RTCP packet");
				return;
			}

			if (!this->srtpSendSession->EncryptRTCP(&data, &len))
			{
				MS_WARN("error encrypting RTCP packet: %s", this->srtpSendSession->GetLastErrorDesc());
				return;
			}

			MS_DEBUG("sending SRTCP packet of %zu bytes", len);
		}
		else
		{
			MS_DEBUG("sending RTCP packet of %zu bytes", len);
		}

		this->sendingSource->Send(data, len);
	}

	// TODO: do it
	void Transport::Dump()
	{
		MS_DEBUG("%zu valid sources:", this->sources.size());
		for (auto it = this->sources.begin(); it != this->sources.end(); ++it)
		{
			RTC::TransportSource* source = &(*it);

			source->Dump();
		}

		if (this->sendingSource)
		{
			MS_DEBUG("sending source:");
			this->sendingSource->Dump();
		}

		if (this->dtlsHandler)
			this->dtlsHandler->Dump();
	}

	inline
	void Transport::RunDTLSHandlerIfReady()
	{
		MS_TRACE();

		if (!this->dtlsHandler->IsRunning() && this->sendingSource)
		{
			switch (this->dtlsRole)
			{
				// If we are DTLS server then run the DTLS handler upon first valid ICE pair.
				case RTC::DTLSRole::SERVER:
					if (this->isIcePaired)
					{
						MS_DEBUG("running DTLS handler in server role");
						this->dtlsHandler->Run(this->dtlsRole);
					}
					break;
				// If we are DTLS client then wait for the first Binding with USE-CANDIDATE.
				case RTC::DTLSRole::CLIENT:
					if (this->isIcePairedWithUseCandidate)
					{
						MS_DEBUG("running DTLS handler in client role");
						this->dtlsHandler->Run(this->dtlsRole);
					}
					break;
				case RTC::DTLSRole::NONE:
					break;
			}
		}
	}

	inline
	void Transport::onSTUNDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!HasFlagICE())
		{
			MS_DEBUG("no ICE flag, ignoring STUN message received via %s", TRANSPORT_NAME(source));
			return;
		}

		RTC::STUNMessage* msg = RTC::STUNMessage::Parse(data, len);
		if (!msg)
		{
			MS_DEBUG("ignoring wrong STUN message received via %s", TRANSPORT_NAME(source));
			return;
		}

		MS_DEBUG("STUN message received via %s, passing it to the ICEServer", TRANSPORT_NAME(source));
		// msg->Dump();

		this->iceServer->ProcessSTUNMessage(msg, source);

		delete msg;
	}

	inline
	void Transport::onDTLSDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!HasFlagDTLS())
		{
			MS_DEBUG("no DTLS flag, ignoring DTLS data received via %s", TRANSPORT_NAME(source));
			return;
		}

		if (!IsValidSource(source))
		{
			MS_DEBUG("ignoring DTLS data coming from an invalid %s address", TRANSPORT_NAME(source));
			return;
		}

		if (this->dtlsHandler->IsRunning())
		{
			MS_DEBUG("DTLS data received via %s, passing it to the DTLSHandler", TRANSPORT_NAME(source));
			this->dtlsHandler->ProcessDTLSData(data, len);
		}
		else
		{
			MS_DEBUG("no DTLSHandler running, ignoring DTLS data received via %s", TRANSPORT_NAME(source));
			return;
		}
	}

	inline
	void Transport::onRTPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!IsValidSource(source))
		{
			MS_DEBUG("ignoring RTP packet coming from an invalid %s address", TRANSPORT_NAME(source));
			return;
		}

		if (HasFlagSRTP())
		{
			if (!this->srtpRecvSession)
			{
				MS_DEBUG("SRTP receiving session not set, ignoring received RTP packet via %s", TRANSPORT_NAME(source));
				return;
			}

			if (!this->srtpRecvSession->DecryptSRTP(data, &len))
			{
				MS_WARN("error decrypting SRTP packet received via %s: %s", TRANSPORT_NAME(source), this->srtpRecvSession->GetLastErrorDesc());
				return;
			}
		}

		RTC::RTPPacket* packet = RTC::RTPPacket::Parse(data, len);
		if (!packet)
		{
			MS_DEBUG("data received via %s is not a valid RTP packet", TRANSPORT_NAME(source));
			return;
		}
		// MS_DEBUG("valid RTP packet received via %s [ssrc: %llu | payload: %hu | size: %zu]",
			// TRANSPORT_NAME(source), (unsigned long long)packet->GetSSRC(), (unsigned short)packet->GetPayloadType(), len);

		// Pass the RTP packet to the listener.
		this->listener->onRTPPacket(this, packet);

		delete packet;
	}

	inline
	void Transport::onRTCPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!IsValidSource(source))
		{
			MS_DEBUG("ignoring RTCP packet coming from an invalid %s address", TRANSPORT_NAME(source));
			return;
		}

		if (HasFlagSRTP())
		{
			if (!this->srtpRecvSession)
			{
				MS_DEBUG("SRTP receiving session not set, ignoring received RTCP packet via %s", TRANSPORT_NAME(source));
				return;
			}

			if (!this->srtpRecvSession->DecryptSRTCP(data, &len))
			{
				MS_DEBUG("error decrypting SRTCP packet received via %s: %s", TRANSPORT_NAME(source), this->srtpRecvSession->GetLastErrorDesc());
				return;
			}
		}

		RTC::RTCPPacket* packet = RTC::RTCPPacket::Parse(data, len);
		if (!packet)
		{
			MS_DEBUG("data received via %s is not a valid RTCP packet", TRANSPORT_NAME(source));
			return;
		}
		// MS_DEBUG("valid RTCP packet received via %s", TRANSPORT_NAME(source));

		// Pass the RTP packet to the listener.
		this->listener->onRTCPPacket(this, packet);

		delete packet;
	}

	void Transport::onSTUNDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onSTUNDataRecv(&source, data, len);
	}

	void Transport::onDTLSDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onDTLSDataRecv(&source, data, len);
	}

	void Transport::onRTPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onRTPDataRecv(&source, data, len);
	}

	void Transport::onRTCPDataRecv(RTC::UDPSocket *socket, const MS_BYTE* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportSource source(socket, remote_addr);
		onRTCPDataRecv(&source, data, len);
	}

	void Transport::onRTCTCPConnectionClosed(RTC::TCPServer* tcpServer, RTC::TCPConnection* connection, bool is_closed_by_peer)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);

		if (RemoveSource(&source))
		{
			MS_DEBUG("valid source %s:", is_closed_by_peer ? "closed by peer" : "locally closed");
			source.Dump();
		}
	}

	void Transport::onSTUNDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onSTUNDataRecv(&source, data, len);
	}

	void Transport::onDTLSDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onDTLSDataRecv(&source, data, len);
	}

	void Transport::onRTPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onRTPDataRecv(&source, data, len);
	}

	void Transport::onRTCPDataRecv(RTC::TCPConnection *connection, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportSource source(connection);
		onRTCPDataRecv(&source, data, len);
	}

	void Transport::onOutgoingSTUNMessage(RTC::ICEServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source)
	{
		MS_TRACE();

		MS_DEBUG("sending STUN message to:");
		source->Dump();
		// msg->Dump();

		source->Send(msg->GetRaw(), msg->GetLength());
	}

	void Transport::onICEValidPair(RTC::ICEServer* iceServer, RTC::TransportSource* source, bool has_use_candidate)
	{
		MS_TRACE();

		this->isIcePaired = true;
		if (has_use_candidate)
			this->isIcePairedWithUseCandidate = true;

		/*
		 * RFC 5245 section 11.2 "Receiving Media":
		 *
		 * ICE implementations MUST be prepared to receive media on each component
		 * on any candidates provided for that component.
		 */
		if (SetSendingSource(source))
		{
			MS_DEBUG("ICE paired");
			Dump();
		}

		// If ready, run the DTLS handler.
		if (HasFlagDTLS())
			RunDTLSHandlerIfReady();
	}

	void Transport::onOutgoingDTLSData(RTC::DTLSHandler* dtlsHandler, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!this->sendingSource)
		{
			MS_WARN("no sending address set, cannot send DTLS packet");
			return;
		}

		MS_DEBUG("sending DTLS data to to:");
		this->sendingSource->Dump();

		this->sendingSource->Send(data, len);
	}

	void Transport::onDTLSConnected(RTC::DTLSHandler* dtlsHandler)
	{
		MS_TRACE();

		MS_DEBUG("DTLS connected");
		Dump();
	}

	void Transport::onDTLSDisconnected(RTC::DTLSHandler* dtlsHandler)
	{
		MS_TRACE();

		MS_DEBUG("DTLS disconnected");

		Reset();
		Dump();
	}

	void Transport::onDTLSFailed(RTC::DTLSHandler* dtlsHandler)
	{
		MS_TRACE();

		MS_DEBUG("DTLS failed");

		Reset();
		Dump();
	}

	void Transport::onSRTPKeyMaterial(RTC::DTLSHandler* dtlsHandler, RTC::SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len)
	{
		MS_TRACE();

		MS_DEBUG("got SRTP key material");

		SetLocalSRTPKey(srtp_profile, srtp_local_key, srtp_local_key_len);
		SetRemoteSRTPKey(srtp_profile, srtp_remote_key, srtp_remote_key_len);
	}

	void Transport::onDTLSApplicationData(RTC::DTLSHandler* dtlsHandler, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		MS_DEBUG("DTLS application data received | size: %zu", len);

		// TMP
		MS_DEBUG("data: %s", std::string((char*)data, len).c_str());

		// TMP
		dtlsHandler->SendApplicationData(data, len);
	}
}
