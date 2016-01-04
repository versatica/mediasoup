#define MS_CLASS "RTC::Transport"

#include "RTC/Transport.h"
#include "Settings.h"
#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <cmath>  // std::pow()

#define ICE_CANDIDATE_PRIORITY_IPV4 20000
#define ICE_CANDIDATE_PRIORITY_IPV6 20000
#define ICE_CANDIDATE_PRIORITY_UDP  10000
#define ICE_CANDIDATE_PRIORITY_TCP   5000
#define ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTP  \
	(std::pow(2, 24) * 64) + (std::pow(2, 8) * (ICE_CANDIDATE_PRIORITY_IPV4 + ICE_CANDIDATE_PRIORITY_UDP)) + (256 - 1)
#define ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTP  \
	(std::pow(2, 24) * 64) + (std::pow(2, 8) * (ICE_CANDIDATE_PRIORITY_IPV6 + ICE_CANDIDATE_PRIORITY_UDP)) + (256 - 1)
#define ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTP  \
	(std::pow(2, 24) * 64) + (std::pow(2, 8) * (ICE_CANDIDATE_PRIORITY_IPV4 + ICE_CANDIDATE_PRIORITY_TCP)) + (256 - 1)
#define ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTP  \
	(std::pow(2, 24) * 64) + (std::pow(2, 8) * (ICE_CANDIDATE_PRIORITY_IPV6 + ICE_CANDIDATE_PRIORITY_TCP)) + (256 - 1)
#define ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTCP ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTP - 1
#define ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTCP ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTP - 1
#define ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTCP ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTP - 1
#define ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTCP ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTP - 1

namespace RTC
{
	/* Class variables. */

	size_t Transport::maxSources = 8;

	/* Instance methods. */

	Transport::Transport(Listener* listener, unsigned int transportId, Json::Value& data, Transport* rtpTransport) :
		transportId(transportId),
		listener(listener)
	{
		MS_TRACE();

		static const Json::StaticString k_udp("udp");
		static const Json::StaticString k_tcp("tcp");

		bool try_IPv4_udp = true;
		bool try_IPv6_udp = true;
		bool try_IPv4_tcp = true;
		bool try_IPv6_tcp = true;

		// RTP transport.
		if (!rtpTransport)
		{
			this->iceComponent = IceComponent::RTP;

			// Create a ICE server.
			this->iceServer = new RTC::IceServer(this,
				Utils::Crypto::GetRandomString(16),
				Utils::Crypto::GetRandomString(32));

			if (data[k_udp].isBool())
				try_IPv4_udp = try_IPv6_udp = data[k_udp].asBool();

			if (data[k_tcp].isBool())
				try_IPv4_tcp = try_IPv6_tcp = data[k_tcp].asBool();
		}
		// RTCP transport associated to a given RTP transport.
		else
		{
			this->iceComponent = IceComponent::RTCP;

			// Create a ICE server.
			this->iceServer = new RTC::IceServer(this,
				rtpTransport->GetIceUsernameFragment(),
				rtpTransport->GetIcePassword());

			try_IPv4_udp = rtpTransport->hasIPv4udp;
			try_IPv6_udp = rtpTransport->hasIPv6udp;
			try_IPv4_tcp = rtpTransport->hasIPv4tcp;
			try_IPv6_tcp = rtpTransport->hasIPv6tcp;
		}

		// Open a IPv4 UDP socket.
		if (try_IPv4_udp && Settings::configuration.hasIPv4)
		{
			unsigned long priority;

			if (this->iceComponent == IceComponent::RTP)
				priority = ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTP;
			else
				priority = ICE_CANDIDATE_PRIORITY_IPV4_UDP_RTCP;

			try
			{
				RTC::UDPSocket* udpSocket = RTC::UDPSocket::Factory(this, AF_INET);
				RTC::IceCandidate iceCandidate(udpSocket, priority);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
				this->hasIPv4udp = true;
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv4 UDP socket: %s", error.what());
			}
		}

		// Open a IPv6 UDP socket.
		if (try_IPv6_udp && Settings::configuration.hasIPv6)
		{
			unsigned long priority;

			if (this->iceComponent == IceComponent::RTP)
				priority = ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTP;
			else
				priority = ICE_CANDIDATE_PRIORITY_IPV6_UDP_RTCP;

			try
			{
				RTC::UDPSocket* udpSocket = RTC::UDPSocket::Factory(this, AF_INET6);
				RTC::IceCandidate iceCandidate(udpSocket, priority);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
				this->hasIPv6udp = true;
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv6 UDP socket: %s", error.what());
			}
		}

		// Open a IPv4 TCP server.
		if (try_IPv4_tcp && Settings::configuration.hasIPv4)
		{
			unsigned long priority;

			if (this->iceComponent == IceComponent::RTP)
				priority = ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTP;
			else
				priority = ICE_CANDIDATE_PRIORITY_IPV4_TCP_RTCP;

			try
			{
				RTC::TCPServer* tcpServer = RTC::TCPServer::Factory(this, this, AF_INET);
				RTC::IceCandidate iceCandidate(tcpServer, priority);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
				this->hasIPv4tcp = true;
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv4 TCP server: %s", error.what());
			}
		}

		// Open a IPv6 TCP server.
		if (try_IPv6_tcp && Settings::configuration.hasIPv6)
		{
			unsigned long priority;

			if (this->iceComponent == IceComponent::RTP)
				priority = ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTP;
			else
				priority = ICE_CANDIDATE_PRIORITY_IPV6_TCP_RTCP;

			try
			{
				RTC::TCPServer* tcpServer = RTC::TCPServer::Factory(this, this, AF_INET6);
				RTC::IceCandidate iceCandidate(tcpServer, priority);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
				this->hasIPv6tcp = true;
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv6 TCP server: %s", error.what());
			}
		}

		// Ensure there is at least one IP:port binding.
		if (!this->udpSockets.size() && !this->tcpServers.size())
		{
			Close();

			MS_THROW_ERROR("could not open any IP:port");
		}

		// Create a DTLS agent.
		this->dtlsTransport = new RTC::DTLSTransport(this);

		// Hack to avoid that Close() above attempts to delete this.
		this->allocated = true;
	}

	Transport::~Transport()
	{
		MS_TRACE();
	}

	void Transport::Close()
	{
		MS_TRACE();

		if (this->iceServer)
			this->iceServer->Close();

		if (this->dtlsTransport)
			this->dtlsTransport->Close();

		// if (this->srtpRecvSession)
			// this->srtpRecvSession->Close();

		// if (this->srtpSendSession)
			// this->srtpSendSession->Close();

		for (auto socket : this->udpSockets)
			socket->Close();

		for (auto server : this->tcpServers)
			server->Close();

		if (this->allocated)
		{
			// Notify the listener.
			this->listener->onTransportClosed(this);

			delete this;
		}
	}

	Json::Value Transport::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_iceComponent("iceComponent");
		static const Json::StaticString k_iceLocalParameters("iceLocalParameters");
		static const Json::StaticString k_usernameFragment("usernameFragment");
		static const Json::StaticString k_password("password");
		static const Json::StaticString k_iceLocalCandidates("iceLocalCandidates");
		static const Json::StaticString v_RTP("RTP");
		static const Json::StaticString v_RTCP("RTCP");
		static const Json::StaticString k_dtlsLocalFingerprints("dtlsLocalFingerprints");

		Json::Value data;

		// Add `iceComponent`.
		if (this->iceComponent == IceComponent::RTP)
			data[k_iceComponent] = v_RTP;
		else
			data[k_iceComponent] = v_RTCP;

		// Add `iceLocalParameters`.
		data[k_iceLocalParameters][k_usernameFragment] = this->iceServer->GetUsernameFragment();
		data[k_iceLocalParameters][k_password] = this->iceServer->GetPassword();

		// Add `iceLocalCandidates`.
		data[k_iceLocalCandidates] = Json::arrayValue;
		for (auto iceCandidate : this->iceLocalCandidates)
		{
			data[k_iceLocalCandidates].append(iceCandidate.toJson());
		}

		// Add `dtlsLocalFingerprints`.
		data[k_dtlsLocalFingerprints] = RTC::DTLSTransport::GetLocalFingerprints();

		return data;
	}

	void Transport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::transport_close:
			{
				unsigned int transportId = this->transportId;

				Close();

				MS_DEBUG("Transport closed [transportId:%u]", transportId);
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::transport_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::transport_setRemoteDtlsParameters:
			{
				static const Json::StaticString k_role("role");
				static const Json::StaticString v_auto("auto");
				static const Json::StaticString v_client("client");
				static const Json::StaticString v_server("server");
				static const Json::StaticString k_fingerprint("fingerprint");
				static const Json::StaticString k_algorithm("algorithm");
				static const Json::StaticString k_value("value");

				RTC::DTLSTransport::Fingerprint remoteFingerprint;
				RTC::DTLSTransport::Role remoteRole = RTC::DTLSTransport::Role::AUTO;  // Default value if missing.

				// Ensure this method is not called twice.
				if (this->remoteDtlsParametersGiven)
				{
					MS_ERROR("method already called");

					request->Reject(500, "method already called");
					return;
				}
				this->remoteDtlsParametersGiven = true;

				// Validate request data.

				if (!request->data[k_fingerprint].isObject())
				{
					MS_ERROR("missing .fingerprint");

					request->Reject(500, "missing .fingerprint");
					return;
				}

				if (!request->data[k_fingerprint][k_algorithm].isString() ||
					  !request->data[k_fingerprint][k_value].isString())
				{
					MS_ERROR("missing .fingerprint.algorithm and/or .fingerprint.value");

					request->Reject(500, "missing .fingerprint.algorithm and/or .fingerprint.value");
					return;
				}

				remoteFingerprint.algorithm = RTC::DTLSTransport::GetFingerprintAlgorithm(request->data[k_fingerprint][k_algorithm].asString());

				if (remoteFingerprint.algorithm == RTC::DTLSTransport::FingerprintAlgorithm::NONE)
				{
					MS_ERROR("unsupported .fingerprint.algorithm");

					request->Reject(500, "unsupported .fingerprint.algorithm");
					return;
				}

				remoteFingerprint.value = request->data[k_fingerprint][k_value].asString();

				if (request->data[k_role].isString())
					remoteRole = RTC::DTLSTransport::GetRole(request->data[k_role].asString());

				// Set local DTLS role.
				switch (remoteRole)
				{
					case RTC::DTLSTransport::Role::CLIENT:
						this->dtlsLocalRole = RTC::DTLSTransport::Role::SERVER;
						break;
					case RTC::DTLSTransport::Role::SERVER:
						this->dtlsLocalRole = RTC::DTLSTransport::Role::CLIENT;
						break;
					case RTC::DTLSTransport::Role::AUTO:
						this->dtlsLocalRole = RTC::DTLSTransport::Role::CLIENT;
						break;
					case RTC::DTLSTransport::Role::NONE:
						MS_ERROR("invalid .role");

						request->Reject(500, "invalid .role");
						return;
				}

				// TODO: Provide the local DTLS role in the response so the JS can expose it.
				Json::Value data;

				switch (this->dtlsLocalRole)
				{
					case RTC::DTLSTransport::Role::CLIENT:
						data["localDtlsRole"] = "client";
						break;
					case RTC::DTLSTransport::Role::SERVER:
						data["localDtlsRole"] = "server";
						break;
					default:
						MS_ABORT("KAKAKAKAKAKA");  // TODO jeje
				}

				request->Accept(data);

				// Pass the remote fingerprint to the DTLS transport.
				this->dtlsTransport->SetRemoteFingerprint(remoteFingerprint);

				// Run the DTLS transport if ready.
				RunDTLSTransportIfReady();

				break;
			}

			default:
			{
				MS_ABORT("unknown method");
			}
		}
	}

	Transport* Transport::CreateAssociatedTransport(unsigned int transportId)
	{
		MS_TRACE();

		static Json::Value nullData(Json::nullValue);

		if (this->iceComponent != IceComponent::RTP)
			MS_THROW_ERROR("cannot call CreateAssociatedTransport() on a RTCP Transport");

		return new RTC::Transport(this->listener, transportId, nullData, this);
	}

	inline
	void Transport::RunDTLSTransportIfReady()
	{
		MS_TRACE();

		switch (this->dtlsLocalRole)
		{
			// If 'client' then wait for the first Binding with USE-CANDIDATE.
			// If 'auto' then behave as 'client' since we are always ICE controlled.
			case RTC::DTLSTransport::Role::CLIENT:
			case RTC::DTLSTransport::Role::AUTO:
				if (this->icePairedWithUseCandidate)
				{
					MS_DEBUG("running DTLS transport in 'client' role");

					this->dtlsTransport->Run(RTC::DTLSTransport::Role::CLIENT);
				}
				break;

			// If 'server' then run the DTLS handler upon first valid ICE pair.
			case RTC::DTLSTransport::Role::SERVER:
				if (this->icePaired)
				{
					MS_DEBUG("running DTLS transport in 'server' role");

					this->dtlsTransport->Run(RTC::DTLSTransport::Role::SERVER);
				}
				break;

			case RTC::DTLSTransport::Role::NONE:
				MS_ABORT("local DTLS role cannot be none");
		}
	}

	inline
	void Transport::onSTUNDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		RTC::STUNMessage* msg = RTC::STUNMessage::Parse(data, len);
		if (!msg)
		{
			MS_DEBUG("ignoring wrong STUN message received");
			return;
		}

		// Pass it to the IceServer.
		this->iceServer->ProcessSTUNMessage(msg, source);

		delete msg;
	}

	inline
	void Transport::onDTLSDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!IsValidSource(source))
		{
			MS_DEBUG("ignoring DTLS data coming from an invalid source");

			return;
		}

		if (this->dtlsTransport->IsRunning())
		{
			MS_DEBUG("DTLS data received, passing it to the DTLS transport");

			this->dtlsTransport->ProcessDTLSData(data, len);
		}
		else
		{
			MS_DEBUG("DTLSTransport is not running, ignoring received DTLS data");

			return;
		}
	}

	inline
	void Transport::onRTPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
	}

	inline
	void Transport::onRTCPDataRecv(RTC::TransportSource* source, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// TODO
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

	void Transport::onOutgoingSTUNMessage(RTC::IceServer* iceServer, RTC::STUNMessage* msg, RTC::TransportSource* source)
	{
		MS_TRACE();

		// Send the STUN response over the same transport source.
		source->Send(msg->GetRaw(), msg->GetLength());
	}

	void Transport::onICEValidPair(RTC::IceServer* iceServer, RTC::TransportSource* source, bool has_use_candidate)
	{
		MS_TRACE();

		/*
		 * RFC 5245 section 11.2 "Receiving Media":
		 *
		 * ICE implementations MUST be prepared to receive media on each component
		 * on any candidates provided for that component.
		 */

		this->icePaired = true;
		if (has_use_candidate)
			this->icePairedWithUseCandidate = true;

		switch (SetSendingSource(source))
		{
			// This is a new source.
			case 0:
				MS_DEBUG("got a new ICE valid pair [UseCandidate:%s]", has_use_candidate ? "true" : "false");
				break;

			// This is already the sending source.
			case 1:
				break;

			// This is another valid source.
			case 2:
				MS_DEBUG("got a previous ICE valid pair [UseCandidate:%s]", has_use_candidate ? "true" : "false");
				break;
		}

		// If ready, run the DTLS handler.
		RunDTLSTransportIfReady();
	}

	void Transport::onOutgoingDTLSData(RTC::DTLSTransport* dtlsTransport, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!this->sendingSource)
		{
			MS_DEBUG("no sending address set, cannot send DTLS packet");
			return;
		}

		// TODO: remove
		MS_DEBUG("sending DTLS data to to:");
		this->sendingSource->Dump();

		this->sendingSource->Send(data, len);
	}

	void Transport::onDTLSConnected(RTC::DTLSTransport* dtlsTransport)
	{
		MS_TRACE();

		MS_DEBUG("DTLS connected");
	}

	void Transport::onDTLSDisconnected(RTC::DTLSTransport* dtlsTransport)
	{
		MS_TRACE();

		MS_DEBUG("DTLS disconnected");

		// TODO
		// Reset();
	}

	void Transport::onDTLSFailed(RTC::DTLSTransport* dtlsTransport)
	{
		MS_TRACE();

		MS_DEBUG("DTLS failed");

		// TODO
		// Reset();
	}

	void Transport::onSRTPKeyMaterial(RTC::DTLSTransport* dtlsTransport, RTC::SRTPSession::SRTPProfile srtp_profile, MS_BYTE* srtp_local_key, size_t srtp_local_key_len, MS_BYTE* srtp_remote_key, size_t srtp_remote_key_len)
	{
		MS_TRACE();

		MS_DEBUG("got SRTP key material");

		// TODO
		// SetLocalSRTPKey(srtp_profile, srtp_local_key, srtp_local_key_len);
		// SetRemoteSRTPKey(srtp_profile, srtp_remote_key, srtp_remote_key_len);
	}

	void Transport::onDTLSApplicationData(RTC::DTLSTransport* dtlsTransport, const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		MS_DEBUG("DTLS application data received [size:%zu]", len);

		// TMP
		MS_DEBUG("data: %s", std::string((char*)data, len).c_str());
	}
}
