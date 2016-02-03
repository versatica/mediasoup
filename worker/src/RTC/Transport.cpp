#define MS_CLASS "RTC::Transport"

#include "RTC/Transport.h"
#include "Settings.h"
#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <cmath>  // std::pow()

#define ICE_CANDIDATE_DEFAULT_LOCAL_PRIORITY 20000
#define ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_FAMILY_INCREMENT 10000
#define ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_PROTOCOL_INCREMENT 5000

/* Static helpers. */

static inline
uint32_t generateIceCandidatePriority(uint16_t local_preference)
{
	MS_TRACE();

	// We just provide 'host' candidates so `type preference` is fixed.
	static uint16_t type_preference = 64;
	// We do not support non rtcp-mux so `component` is always 1.
	static uint16_t component = 1;

	return
		std::pow(2, 24) * type_preference  +
		std::pow(2,  8) * local_preference +
		std::pow(2,  0) * (256 - component);
}

namespace RTC
{
	/* Instance methods. */

	Transport::Transport(Listener* listener, Channel::Notifier* notifier, uint32_t transportId, Json::Value& data) :
		transportId(transportId),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();

		static const Json::StaticString k_udp("udp");
		static const Json::StaticString k_tcp("tcp");
		static const Json::StaticString k_preferIPv4("preferIPv4");
		static const Json::StaticString k_preferIPv6("preferIPv6");
		static const Json::StaticString k_preferUdp("preferUdp");
		static const Json::StaticString k_preferTcp("preferTcp");

		bool try_IPv4_udp = true;
		bool try_IPv6_udp = true;
		bool try_IPv4_tcp = true;
		bool try_IPv6_tcp = true;

		bool preferIPv4 = false;
		bool preferIPv6 = false;
		bool preferUdp = false;
		bool preferTcp = false;

		if (data[k_udp].isBool())
			try_IPv4_udp = try_IPv6_udp = data[k_udp].asBool();

		if (data[k_tcp].isBool())
			try_IPv4_tcp = try_IPv6_tcp = data[k_tcp].asBool();

		if (data[k_preferIPv4].isBool())
			preferIPv4 = data[k_preferIPv4].asBool();
		if (data[k_preferIPv6].isBool())
			preferIPv6 = data[k_preferIPv6].asBool();
		if (data[k_preferUdp].isBool())
			preferUdp = data[k_preferUdp].asBool();
		if (data[k_preferTcp].isBool())
			preferTcp = data[k_preferTcp].asBool();

		// Create a ICE server.
		this->iceServer = new RTC::ICEServer(this,
			Utils::Crypto::GetRandomString(16),
			Utils::Crypto::GetRandomString(32));

		// Open a IPv4 UDP socket.
		if (try_IPv4_udp && Settings::configuration.hasIPv4)
		{
			uint16_t local_preference = ICE_CANDIDATE_DEFAULT_LOCAL_PRIORITY;

			if (preferIPv4)
				local_preference += ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_FAMILY_INCREMENT;
			if (preferUdp)
				local_preference += ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_PROTOCOL_INCREMENT;

			uint32_t priority = generateIceCandidatePriority(local_preference);

			try
			{
				RTC::UDPSocket* udpSocket = new RTC::UDPSocket(this, AF_INET);
				RTC::IceCandidate iceCandidate(udpSocket, priority);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv4 UDP socket: %s", error.what());
			}
		}

		// Open a IPv6 UDP socket.
		if (try_IPv6_udp && Settings::configuration.hasIPv6)
		{
			uint16_t local_preference = ICE_CANDIDATE_DEFAULT_LOCAL_PRIORITY;

			if (preferIPv6)
				local_preference += ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_FAMILY_INCREMENT;
			if (preferUdp)
				local_preference += ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_PROTOCOL_INCREMENT;

			uint32_t priority = generateIceCandidatePriority(local_preference);

			try
			{
				RTC::UDPSocket* udpSocket = new RTC::UDPSocket(this, AF_INET6);
				RTC::IceCandidate iceCandidate(udpSocket, priority);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv6 UDP socket: %s", error.what());
			}
		}

		// Open a IPv4 TCP server.
		if (try_IPv4_tcp && Settings::configuration.hasIPv4)
		{
			uint16_t local_preference = ICE_CANDIDATE_DEFAULT_LOCAL_PRIORITY;

			if (preferIPv4)
				local_preference += ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_FAMILY_INCREMENT;
			if (preferTcp)
				local_preference += ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_PROTOCOL_INCREMENT;

			uint32_t priority = generateIceCandidatePriority(local_preference);

			try
			{
				RTC::TCPServer* tcpServer = new RTC::TCPServer(this, this, AF_INET);
				RTC::IceCandidate iceCandidate(tcpServer, priority);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError &error)
			{
				MS_ERROR("error adding IPv4 TCP server: %s", error.what());
			}
		}

		// Open a IPv6 TCP server.
		if (try_IPv6_tcp && Settings::configuration.hasIPv6)
		{
			uint16_t local_preference = ICE_CANDIDATE_DEFAULT_LOCAL_PRIORITY;

			if (preferIPv6)
				local_preference += ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_FAMILY_INCREMENT;
			if (preferTcp)
				local_preference += ICE_CANDIDATE_LOCAL_PRIORITY_PREFER_PROTOCOL_INCREMENT;

			uint32_t priority = generateIceCandidatePriority(local_preference);

			try
			{
				RTC::TCPServer* tcpServer = new RTC::TCPServer(this, this, AF_INET6);
				RTC::IceCandidate iceCandidate(tcpServer, priority);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
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

		static const Json::StaticString k_class("class");

		Json::Value event_data(Json::objectValue);

		// if (this->srtpRecvSession)
		// 	this->srtpRecvSession->Close();

		// if (this->srtpSendSession)
		// 	this->srtpSendSession->Close();

		if (this->dtlsTransport)
			this->dtlsTransport->Close();

		if (this->iceServer)
			this->iceServer->Close();

		for (auto socket : this->udpSockets)
			socket->Close();
		this->udpSockets.clear();

		for (auto server : this->tcpServers)
			server->Close();
		this->tcpServers.clear();

		this->selectedTuple = nullptr;

		// Notify.
		event_data[k_class] = "Transport";
		this->notifier->Emit(this->transportId, "close", event_data);

		// If this was allocated (it did not throw in the constructor) notify the
		// listener and delete it.
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

		static const Json::StaticString k_iceRole("iceRole");
		static const Json::StaticString v_controlled("controlled");
		static const Json::StaticString k_iceLocalParameters("iceLocalParameters");
		static const Json::StaticString k_usernameFragment("usernameFragment");
		static const Json::StaticString k_password("password");
		static const Json::StaticString k_iceLocalCandidates("iceLocalCandidates");
		static const Json::StaticString k_iceSelectedTuple("iceSelectedTuple");
		static const Json::StaticString k_iceState("iceState");
		static const Json::StaticString v_new("new");
		static const Json::StaticString v_connected("connected");
		static const Json::StaticString v_completed("completed");
		static const Json::StaticString v_disconnected("disconnected");
		static const Json::StaticString k_dtlsLocalParameters("dtlsLocalParameters");
		static const Json::StaticString k_fingerprints("fingerprints");
		static const Json::StaticString k_role("role");
		static const Json::StaticString v_auto("auto");
		static const Json::StaticString v_client("client");
		static const Json::StaticString v_server("server");
		static const Json::StaticString k_dtlsState("dtlsState");
		static const Json::StaticString v_connecting("connecting");
		static const Json::StaticString v_closed("closed");
		static const Json::StaticString v_failed("failed");
		static const Json::StaticString k_rtpListener("rtpListener");

		Json::Value json(Json::objectValue);

		// Add `iceRole` (we are always "controlled").
		json[k_iceRole] = v_controlled;

		// Add `iceLocalParameters`.
		json[k_iceLocalParameters][k_usernameFragment] = this->iceServer->GetUsernameFragment();
		json[k_iceLocalParameters][k_password] = this->iceServer->GetPassword();

		// Add `iceLocalCandidates`.
		json[k_iceLocalCandidates] = Json::arrayValue;
		for (auto iceCandidate : this->iceLocalCandidates)
		{
			json[k_iceLocalCandidates].append(iceCandidate.toJson());
		}

		// Add `iceSelectedTuple`.
		if (this->selectedTuple)
			json[k_iceSelectedTuple] = this->selectedTuple->toJson();

		// Add `iceState`.
		switch (this->iceServer->GetState())
		{
			case RTC::ICEServer::IceState::NEW:
				json[k_iceState] = v_new;
				break;
			case RTC::ICEServer::IceState::CONNECTED:
				json[k_iceState] = v_connected;
				break;
			case RTC::ICEServer::IceState::COMPLETED:
				json[k_iceState] = v_completed;
				break;
			case RTC::ICEServer::IceState::DISCONNECTED:
				json[k_iceState] = v_disconnected;
				break;
		}

		// Add `dtlsLocalParameters.fingerprints`.
		json[k_dtlsLocalParameters][k_fingerprints] = RTC::DTLSTransport::GetLocalFingerprints();

		// Add `dtlsLocalParameters.role`.
		switch (this->dtlsLocalRole)
		{
			case RTC::DTLSTransport::Role::AUTO:
				json[k_dtlsLocalParameters][k_role] = v_auto;
				break;
			case RTC::DTLSTransport::Role::CLIENT:
				json[k_dtlsLocalParameters][k_role] = v_client;
				break;
			case RTC::DTLSTransport::Role::SERVER:
				json[k_dtlsLocalParameters][k_role] = v_server;
				break;
			default:
				MS_ABORT("invalid local DTLS role");
		}

		// Add `dtlsState`.
		switch (this->dtlsTransport->GetState())
		{
			case DTLSTransport::DtlsState::NEW:
				json[k_dtlsState] = v_new;
				break;
			case DTLSTransport::DtlsState::CONNECTING:
				json[k_dtlsState] = v_connecting;
				break;
			case DTLSTransport::DtlsState::CONNECTED:
				json[k_dtlsState] = v_connected;
				break;
			case DTLSTransport::DtlsState::FAILED:
				json[k_dtlsState] = v_failed;
				break;
			case DTLSTransport::DtlsState::CLOSED:
				json[k_dtlsState] = v_closed;
				break;
		}

		// Add `rtpListener`.
		json[k_rtpListener] = RTC::RtpListener::toJson();

		return json;
	}

	void Transport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::transport_close:
			{
				uint32_t transportId = this->transportId;

				Close();

				MS_DEBUG("Transport closed [transportId:%" PRIu32 "]", transportId);
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

					request->Reject("method already called");
					return;
				}
				this->remoteDtlsParametersGiven = true;

				// Validate request data.

				if (!request->data[k_fingerprint].isObject())
				{
					MS_ERROR("missing `data.fingerprint`");

					request->Reject("missing `data.fingerprint`");
					return;
				}

				if (!request->data[k_fingerprint][k_algorithm].isString() ||
					  !request->data[k_fingerprint][k_value].isString())
				{
					MS_ERROR("missing `data.fingerprint.algorithm` and/or `data.fingerprint.value`");

					request->Reject("missing `data.fingerprint.algorithm` and/or `data.fingerprint.value`");
					return;
				}

				remoteFingerprint.algorithm = RTC::DTLSTransport::GetFingerprintAlgorithm(request->data[k_fingerprint][k_algorithm].asString());

				if (remoteFingerprint.algorithm == RTC::DTLSTransport::FingerprintAlgorithm::NONE)
				{
					MS_ERROR("unsupported `data.fingerprint.algorithm`");

					request->Reject("unsupported `data.fingerprint.algorithm`");
					return;
				}

				remoteFingerprint.value = request->data[k_fingerprint][k_value].asString();

				if (request->data[k_role].isString())
					remoteRole = RTC::DTLSTransport::StringToRole(request->data[k_role].asString());

				// Set local DTLS role.
				switch (remoteRole)
				{
					case RTC::DTLSTransport::Role::CLIENT:
						this->dtlsLocalRole = RTC::DTLSTransport::Role::SERVER;
						break;
					case RTC::DTLSTransport::Role::SERVER:
						this->dtlsLocalRole = RTC::DTLSTransport::Role::CLIENT;
						break;
					// If the peer has "auto" we become "client" since we are ICE controlled.
					case RTC::DTLSTransport::Role::AUTO:
						this->dtlsLocalRole = RTC::DTLSTransport::Role::CLIENT;
						break;
					case RTC::DTLSTransport::Role::NONE:
						MS_ERROR("invalid .role");

						request->Reject("invalid `data.role`");
						return;
				}

				Json::Value data(Json::objectValue);

				switch (this->dtlsLocalRole)
				{
					case RTC::DTLSTransport::Role::CLIENT:
						data[k_role] = v_client;
						break;
					case RTC::DTLSTransport::Role::SERVER:
						data[k_role] = v_server;
						break;
					default:
						MS_ABORT("invalid local DTLS role");
				}

				request->Accept(data);

				// Pass the remote fingerprint to the DTLS transport-
				this->dtlsTransport->SetRemoteFingerprint(remoteFingerprint);

				// Run the DTLS transport if ready.
				MayRunDTLSTransport();

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	inline
	void Transport::MayRunDTLSTransport()
	{
		MS_TRACE();

		// Do nothing if we have the same local DTLS role as the DTLS transport.
		// NOTE: local role in DTLS transport can be NONE, but not ours.
		if (this->dtlsTransport->GetLocalRole() == this->dtlsLocalRole)
			return;

		// Check our local DTLS role.
		switch (this->dtlsLocalRole)
		{
			// If still 'auto' then transition to 'server' if ICE is 'connected' or
			// 'completed'.
			case RTC::DTLSTransport::Role::AUTO:
				if (this->iceServer->GetState() == RTC::ICEServer::IceState::CONNECTED ||
				    this->iceServer->GetState() == RTC::ICEServer::IceState::COMPLETED)
				{
					MS_DEBUG("transition from DTLS local role 'auto' to 'server' and running DTLS transport");

					this->dtlsLocalRole = RTC::DTLSTransport::Role::SERVER;
					this->dtlsTransport->Run(RTC::DTLSTransport::Role::SERVER);
				}
				break;

			// 'client' is only set if a 'setRemoteDtlsParameters' request was previously
			// received with remote DTLS role 'server'.
			// If 'client' then wait for ICE to be 'completed' (got USE-CANDIDATE).
			case RTC::DTLSTransport::Role::CLIENT:
				if (this->iceServer->GetState() == RTC::ICEServer::IceState::COMPLETED)
				{
					MS_DEBUG("running DTLS transport in local role 'client'");

					this->dtlsTransport->Run(RTC::DTLSTransport::Role::CLIENT);
				}
				break;

			// If 'server' then run the DTLS transport if ICE is 'connected' (not yet
			// USE-CANDIDATE) or 'completed'.
			case RTC::DTLSTransport::Role::SERVER:
				if (this->iceServer->GetState() == RTC::ICEServer::IceState::CONNECTED ||
				    this->iceServer->GetState() == RTC::ICEServer::IceState::COMPLETED)
				{
					MS_DEBUG("running DTLS transport in local role 'server'");

					this->dtlsTransport->Run(RTC::DTLSTransport::Role::SERVER);
				}
				break;

			case RTC::DTLSTransport::Role::NONE:
				MS_ABORT("local DTLS role not set");
		}
	}

	inline
	void Transport::onPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Check if it's STUN.
		if (STUNMessage::IsSTUN(data, len))
		{
			onSTUNDataRecv(tuple, data, len);
		}
		// Check if it's RTCP.
		else if (RTCPPacket::IsRTCP(data, len))
		{
			onRTCPDataRecv(tuple, data, len);
		}
		// Check if it's RTP.
		else if (RTPPacket::IsRTP(data, len))
		{
			onRTPDataRecv(tuple, data, len);
		}
		// Check if it's DTLS.
		else if (DTLSTransport::IsDTLS(data, len))
		{
			onDTLSDataRecv(tuple, data, len);
		}
		else
		{
			// TODO: should not debug all the unknown packets.
			MS_DEBUG("ignoring received packet of unknown type");
		}
	}

	inline
	void Transport::onSTUNDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::STUNMessage* msg = RTC::STUNMessage::Parse(data, len);
		if (!msg)
		{
			MS_DEBUG("ignoring wrong STUN message received");

			return;
		}

		// Pass it to the ICEServer.
		this->iceServer->ProcessSTUNMessage(msg, tuple);

		delete msg;
	}

	inline
	void Transport::onDTLSDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!this->iceServer->IsValidTuple(tuple))
		{
			MS_DEBUG("ignoring DTLS data coming from an invalid tuple");

			return;
		}

		// Trick for clients performing aggressive ICE regardless we are ICE-Lite.
		this->iceServer->ForceSelectedTuple(tuple);

		// Check that DTLS status is 'connecting' or 'connected'.
		if (this->dtlsTransport->GetState() == DTLSTransport::DtlsState::CONNECTING ||
		    this->dtlsTransport->GetState() == DTLSTransport::DtlsState::CONNECTED)
		{
			MS_DEBUG("DTLS data received, passing it to the DTLS transport");

			this->dtlsTransport->ProcessDTLSData(data, len);
		}
		else
		{
			MS_DEBUG("DTLSTransport is not 'connecting' or 'conneted', ignoring received DTLS data");

			return;
		}
	}

	inline
	void Transport::onRTPDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// TODO: Shouldn't we check that DTLS is done???

		RTC::RtpReceiver* rtpReceiver = nullptr;

		if (!this->iceServer->IsValidTuple(tuple))
		{
			MS_DEBUG("ignoring RTP data coming from an invalid tuple");

			return;
		}

		// Trick for clients performing aggressive ICE regardless we are ICE-Lite.
		// TODO: Do this just after validating SRTP and so on.
		this->iceServer->ForceSelectedTuple(tuple);

		// TODO
		// MS_DEBUG("received RTP data");

		RTC::RTPPacket* packet = RTC::RTPPacket::Parse(data, len);
		if (!packet)
		{
			MS_DEBUG("data received is not a valid RTP packet");

			return;
		}

		// Get the associated RtpReceiver.
		rtpReceiver = RTC::RtpListener::GetRtpReceiver(packet);

		if (!rtpReceiver)
		{
			MS_WARN("no suitable RtpReceiver for received RTP packet [ssrc:%" PRIu32 ", payload:%" PRIu8 "]", packet->GetSSRC(), packet->GetPayloadType());
		}
		else
		{
			MS_DEBUG("valid RTP packet received [ssrc:%" PRIu32 ", payload:%" PRIu8 ", rtpReceiver:%" PRIu32 "]", packet->GetSSRC(), packet->GetPayloadType(), rtpReceiver->rtpReceiverId);
			// packet->Dump();
		}

		delete packet;
	}

	inline
	void Transport::onRTCPDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!this->iceServer->IsValidTuple(tuple))
		{
			MS_DEBUG("ignoring RTCP data coming from an invalid tuple");

			return;
		}

		// Trick for clients performing aggressive ICE regardless we are ICE-Lite.
		this->iceServer->ForceSelectedTuple(tuple);

		// TODO
		MS_DEBUG("received RTCP data");
	}

	void Transport::onPacketRecv(RTC::UDPSocket *socket, const uint8_t* data, size_t len, const struct sockaddr* remote_addr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remote_addr);

		onPacketRecv(&tuple, data, len);
	}

	void Transport::onRTCTCPConnectionClosed(RTC::TCPServer* tcpServer, RTC::TCPConnection* connection, bool is_closed_by_peer)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		if (is_closed_by_peer)
			this->iceServer->RemoveTuple(&tuple);
	}

	void Transport::onPacketRecv(RTC::TCPConnection *connection, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		onPacketRecv(&tuple, data, len);
	}

	void Transport::onOutgoingSTUNMessage(RTC::ICEServer* iceServer, RTC::STUNMessage* msg, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Send the STUN response over the same transport tuple.
		tuple->Send(msg->GetRaw(), msg->GetLength());
	}

	void Transport::onICESelectedTuple(RTC::ICEServer* iceServer, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		static const Json::StaticString k_iceSelectedTuple("iceSelectedTuple");

		Json::Value event_data(Json::objectValue);

		/*
		 * RFC 5245 section 11.2 "Receiving Media":
		 *
		 * ICE implementations MUST be prepared to receive media on each component
		 * on any candidates provided for that component.
		 */

		// Update the selected tuple.
		this->selectedTuple = tuple;

		// Notify.
		event_data[k_iceSelectedTuple] = tuple->toJson();
		this->notifier->Emit(this->transportId, "iceselectedtuplechange", event_data);
	}

	void Transport::onICEConnected(RTC::ICEServer* iceServer)
	{
		MS_TRACE();

		static const Json::StaticString k_iceState("iceState");
		static const Json::StaticString v_connected("connected");

		Json::Value event_data(Json::objectValue);

		MS_DEBUG("ICE connected");

		// Notify.
		event_data[k_iceState] = v_connected;
		this->notifier->Emit(this->transportId, "icestatechange", event_data);

		// If ready, run the DTLS handler.
		MayRunDTLSTransport();
	}

	void Transport::onICECompleted(RTC::ICEServer* iceServer)
	{
		MS_TRACE();

		static const Json::StaticString k_iceState("iceState");
		static const Json::StaticString v_completed("completed");

		Json::Value event_data(Json::objectValue);

		MS_DEBUG("ICE completed");

		// Notify.
		event_data[k_iceState] = v_completed;
		this->notifier->Emit(this->transportId, "icestatechange", event_data);

		// If ready, run the DTLS handler.
		MayRunDTLSTransport();
	}

	void Transport::onICEDisconnected(RTC::ICEServer* iceServer)
	{
		MS_TRACE();

		static const Json::StaticString k_iceState("iceState");
		static const Json::StaticString v_disconnected("disconnected");

		Json::Value event_data(Json::objectValue);

		MS_DEBUG("ICE disconnected");

		// Unset the selected tuple.
		this->selectedTuple = nullptr;

		// Notify.
		event_data[k_iceState] = v_disconnected;
		this->notifier->Emit(this->transportId, "icestatechange", event_data);

		// This is a fatal error so close the transport.
		Close();
	}

	void Transport::onDTLSConnecting(RTC::DTLSTransport* dtlsTransport)
	{
		MS_TRACE();

		static const Json::StaticString k_dtlsState("dtlsState");
		static const Json::StaticString v_connecting("connecting");

		Json::Value event_data(Json::objectValue);

		MS_DEBUG("DTLS connecting");

		// Notify.
		event_data[k_dtlsState] = v_connecting;
		this->notifier->Emit(this->transportId, "dtlsstatechange", event_data);
	}

	void Transport::onDTLSConnected(RTC::DTLSTransport* dtlsTransport, RTC::SRTPSession::Profile srtp_profile, uint8_t* srtp_local_key, size_t srtp_local_key_len, uint8_t* srtp_remote_key, size_t srtp_remote_key_len)
	{
		MS_TRACE();

		static const Json::StaticString k_dtlsState("dtlsState");
		static const Json::StaticString v_connected("connected");

		Json::Value event_data(Json::objectValue);

		MS_DEBUG("DTLS connected");

		// TODO
		// SetLocalSRTPKey(srtp_profile, srtp_local_key, srtp_local_key_len);
		// SetRemoteSRTPKey(srtp_profile, srtp_remote_key, srtp_remote_key_len);

		// Notify.
		event_data[k_dtlsState] = v_connected;
		this->notifier->Emit(this->transportId, "dtlsstatechange", event_data);
	}

	void Transport::onDTLSFailed(RTC::DTLSTransport* dtlsTransport)
	{
		MS_TRACE();

		static const Json::StaticString k_dtlsState("dtlsState");
		static const Json::StaticString v_failed("failed");

		Json::Value event_data(Json::objectValue);

		MS_DEBUG("DTLS failed");

		// Notify.
		event_data[k_dtlsState] = v_failed;
		this->notifier->Emit(this->transportId, "dtlsstatechange", event_data);

		// This is a fatal error so close the transport.
		Close();
	}

	void Transport::onDTLSClosed(RTC::DTLSTransport* dtlsTransport)
	{
		MS_TRACE();

		static const Json::StaticString k_dtlsState("dtlsState");
		static const Json::StaticString v_closed("closed");

		Json::Value event_data(Json::objectValue);

		MS_DEBUG("DTLS remotely closed");

		// Notify.
		event_data[k_dtlsState] = v_closed;
		this->notifier->Emit(this->transportId, "dtlsstatechange", event_data);

		// This is a fatal error so close the transport.
		Close();
	}

	void Transport::onOutgoingDTLSData(RTC::DTLSTransport* dtlsTransport, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!this->selectedTuple)
		{
			MS_DEBUG("no selected tuple set, cannot send DTLS packet");
			return;
		}

		// TODO: remove
		MS_DEBUG("media DTLS data to to:");
		this->selectedTuple->Dump();

		this->selectedTuple->Send(data, len);
	}

	void Transport::onDTLSApplicationData(RTC::DTLSTransport* dtlsTransport, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		MS_DEBUG("DTLS application data received [size:%zu]", len);

		// TMP
		MS_DEBUG("data: %s", std::string((char*)data, len).c_str());
	}
}
