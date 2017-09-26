#define MS_CLASS "RTC::WebRtcTransport"
// #define MS_LOG_DEV

#include "RTC/WebRtcTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <cmath>    // std::pow()
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream

namespace RTC
{
	/* Static. */

	static constexpr uint16_t IceCandidateDefaultLocalPriority{ 20000 };
	static constexpr uint16_t IceCandidateLocalPriorityPreferFamilyIncrement{ 10000 };
	static constexpr uint16_t IceCandidateLocalPriorityPreferProtocolIncrement{ 5000 };
	// We just provide "host" candidates so type preference is fixed.
	static constexpr uint16_t IceTypePreference{ 64 };
	// We do not support non rtcp-mux so component is always 1.
	static constexpr uint16_t IceComponent{ 1 };
	static constexpr uint64_t EffectiveMaxBitrateCheckInterval{ 2000 }; // In ms.

	static inline uint32_t generateIceCandidatePriority(uint16_t localPreference)
	{
		MS_TRACE();

		return std::pow(2, 24) * IceTypePreference + std::pow(2, 8) * localPreference +
		       std::pow(2, 0) * (256 - IceComponent);
	}

	/* Instance methods. */

	WebRtcTransport::WebRtcTransport(
	  RTC::Transport::Listener* listener,
	  Channel::Notifier* notifier,
	  uint32_t transportId,
	  Options& options)
	  : RTC::Transport::Transport(listener, notifier, transportId)
	{
		MS_TRACE();

		bool tryIPv4udp{ options.udp };
		bool tryIPv6udp{ options.udp };
		bool tryIPv4tcp{ options.tcp };
		bool tryIPv6tcp{ options.tcp };

		// Create a ICE server.
		this->iceServer = new RTC::IceServer(
		  this, Utils::Crypto::GetRandomString(16), Utils::Crypto::GetRandomString(32));

		// Open a IPv4 UDP socket.
		if (tryIPv4udp && Settings::configuration.hasIPv4)
		{
			uint16_t localPreference = IceCandidateDefaultLocalPriority;

			if (options.preferIPv4)
				localPreference += IceCandidateLocalPriorityPreferFamilyIncrement;
			if (options.preferUdp)
				localPreference += IceCandidateLocalPriorityPreferProtocolIncrement;

			uint32_t priority = generateIceCandidatePriority(localPreference);

			try
			{
				auto* udpSocket = new RTC::UdpSocket(this, AF_INET);
				RTC::IceCandidate iceCandidate(udpSocket, priority);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError& error)
			{
				MS_ERROR("error adding IPv4 UDP socket: %s", error.what());
			}
		}

		// Open a IPv6 UDP socket.
		if (tryIPv6udp && Settings::configuration.hasIPv6)
		{
			uint16_t localPreference = IceCandidateDefaultLocalPriority;

			if (options.preferIPv6)
				localPreference += IceCandidateLocalPriorityPreferFamilyIncrement;
			if (options.preferUdp)
				localPreference += IceCandidateLocalPriorityPreferProtocolIncrement;

			uint32_t priority = generateIceCandidatePriority(localPreference);

			try
			{
				auto* udpSocket = new RTC::UdpSocket(this, AF_INET6);
				RTC::IceCandidate iceCandidate(udpSocket, priority);

				this->udpSockets.push_back(udpSocket);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError& error)
			{
				MS_ERROR("error adding IPv6 UDP socket: %s", error.what());
			}
		}

		// Open a IPv4 TCP server.
		if (tryIPv4tcp && Settings::configuration.hasIPv4)
		{
			uint16_t localPreference = IceCandidateDefaultLocalPriority;

			if (options.preferIPv4)
				localPreference += IceCandidateLocalPriorityPreferFamilyIncrement;
			if (options.preferTcp)
				localPreference += IceCandidateLocalPriorityPreferProtocolIncrement;

			uint32_t priority = generateIceCandidatePriority(localPreference);

			try
			{
				auto* tcpServer = new RTC::TcpServer(this, this, AF_INET);
				RTC::IceCandidate iceCandidate(tcpServer, priority);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError& error)
			{
				MS_ERROR("error adding IPv4 TCP server: %s", error.what());
			}
		}

		// Open a IPv6 TCP server.
		if (tryIPv6tcp && Settings::configuration.hasIPv6)
		{
			uint16_t localPreference = IceCandidateDefaultLocalPriority;

			if (options.preferIPv6)
				localPreference += IceCandidateLocalPriorityPreferFamilyIncrement;
			if (options.preferTcp)
				localPreference += IceCandidateLocalPriorityPreferProtocolIncrement;

			uint32_t priority = generateIceCandidatePriority(localPreference);

			try
			{
				auto* tcpServer = new RTC::TcpServer(this, this, AF_INET6);
				RTC::IceCandidate iceCandidate(tcpServer, priority);

				this->tcpServers.push_back(tcpServer);
				this->iceLocalCandidates.push_back(iceCandidate);
			}
			catch (const MediaSoupError& error)
			{
				MS_ERROR("error adding IPv6 TCP server: %s", error.what());
			}
		}

		// Ensure there is at least one IP:port binding.
		if (this->udpSockets.empty() && this->tcpServers.empty())
		{
			delete this;

			MS_THROW_ERROR("could not open any IP:port");
		}

		// Create a DTLS agent.
		this->dtlsTransport = new RTC::DtlsTransport(this);

		// Set remote bitrate estimator.
		this->remoteBitrateEstimator.reset(new RTC::RemoteBitrateEstimatorAbsSendTime(this));

		// Start the RTCP timer.
		this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));
	}

	WebRtcTransport::~WebRtcTransport()
	{
		MS_TRACE();

		if (this->srtpRecvSession != nullptr)
			this->srtpRecvSession->Destroy();

		if (this->srtpSendSession != nullptr)
			this->srtpSendSession->Destroy();

		if (this->dtlsTransport != nullptr)
			this->dtlsTransport->Destroy();

		if (this->iceServer != nullptr)
			this->iceServer->Destroy();

		for (auto* socket : this->udpSockets)
		{
			socket->Destroy();
		}
		this->udpSockets.clear();

		for (auto* server : this->tcpServers)
		{
			server->Destroy();
		}
		this->tcpServers.clear();

		this->selectedTuple = nullptr;
	}

	Json::Value WebRtcTransport::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTransportId{ "transportId" };
		static const Json::StaticString JsonStringIceRole{ "iceRole" };
		static const Json::StaticString JsonStringControlled{ "controlled" };
		static const Json::StaticString JsonStringIceLocalParameters{ "iceLocalParameters" };
		static const Json::StaticString JsonStringUsernameFragment{ "usernameFragment" };
		static const Json::StaticString JsonStringPassword{ "password" };
		static const Json::StaticString JsonStringIceLite{ "iceLite" };
		static const Json::StaticString JsonStringIceLocalCandidates{ "iceLocalCandidates" };
		static const Json::StaticString JsonStringIceSelectedTuple{ "iceSelectedTuple" };
		static const Json::StaticString JsonStringIceState{ "iceState" };
		static const Json::StaticString JsonStringNew{ "new" };
		static const Json::StaticString JsonStringConnected{ "connected" };
		static const Json::StaticString JsonStringCompleted{ "completed" };
		static const Json::StaticString JsonStringDisconnected{ "disconnected" };
		static const Json::StaticString JsonStringDtlsLocalParameters{ "dtlsLocalParameters" };
		static const Json::StaticString JsonStringFingerprints{ "fingerprints" };
		static const Json::StaticString JsonStringRole{ "role" };
		static const Json::StaticString JsonStringAuto{ "auto" };
		static const Json::StaticString JsonStringClient{ "client" };
		static const Json::StaticString JsonStringServer{ "server" };
		static const Json::StaticString JsonStringDtlsState{ "dtlsState" };
		static const Json::StaticString JsonStringConnecting{ "connecting" };
		static const Json::StaticString JsonStringClosed{ "closed" };
		static const Json::StaticString JsonStringFailed{ "failed" };
		static const Json::StaticString JsonStringHeaderExtensionIds{ "headerExtensionIds" };
		static const Json::StaticString JsonStringAbsSendTime{ "absSendTime" };
		static const Json::StaticString JsonStringRid{ "rid" };
		static const Json::StaticString JsonStringMaxBitrate{ "maxBitrate" };
		static const Json::StaticString JsonStringEffectiveMaxBitrate{ "effectiveMaxBitrate" };
		static const Json::StaticString JsonStringRtpListener{ "rtpListener" };

		Json::Value json(Json::objectValue);
		Json::Value jsonHeaderExtensionIds(Json::objectValue);

		json[JsonStringTransportId] = Json::UInt{ this->transportId };

		// Add iceRole (we are always "controlled").
		json[JsonStringIceRole] = JsonStringControlled;

		// Add iceLocalParameters.
		json[JsonStringIceLocalParameters][JsonStringUsernameFragment] =
		  this->iceServer->GetUsernameFragment();
		json[JsonStringIceLocalParameters][JsonStringPassword] = this->iceServer->GetPassword();
		json[JsonStringIceLocalParameters][JsonStringIceLite]  = true;

		// Add iceLocalCandidates.
		json[JsonStringIceLocalCandidates] = Json::arrayValue;
		for (const auto& iceCandidate : this->iceLocalCandidates)
		{
			json[JsonStringIceLocalCandidates].append(iceCandidate.ToJson());
		}

		// Add iceSelectedTuple.
		if (this->selectedTuple != nullptr)
			json[JsonStringIceSelectedTuple] = this->selectedTuple->ToJson();

		// Add iceState.
		switch (this->iceServer->GetState())
		{
			case RTC::IceServer::IceState::NEW:
				json[JsonStringIceState] = JsonStringNew;
				break;
			case RTC::IceServer::IceState::CONNECTED:
				json[JsonStringIceState] = JsonStringConnected;
				break;
			case RTC::IceServer::IceState::COMPLETED:
				json[JsonStringIceState] = JsonStringCompleted;
				break;
			case RTC::IceServer::IceState::DISCONNECTED:
				json[JsonStringIceState] = JsonStringDisconnected;
				break;
		}

		// Add dtlsLocalParameters.fingerprints.
		json[JsonStringDtlsLocalParameters][JsonStringFingerprints] =
		  RTC::DtlsTransport::GetLocalFingerprints();

		// Add dtlsLocalParameters.role.
		switch (this->dtlsLocalRole)
		{
			case RTC::DtlsTransport::Role::AUTO:
				json[JsonStringDtlsLocalParameters][JsonStringRole] = JsonStringAuto;
				break;
			case RTC::DtlsTransport::Role::CLIENT:
				json[JsonStringDtlsLocalParameters][JsonStringRole] = JsonStringClient;
				break;
			case RTC::DtlsTransport::Role::SERVER:
				json[JsonStringDtlsLocalParameters][JsonStringRole] = JsonStringServer;
				break;
			default:
				MS_ABORT("invalid local DTLS role");
		}

		// Add dtlsState.
		switch (this->dtlsTransport->GetState())
		{
			case DtlsTransport::DtlsState::NEW:
				json[JsonStringDtlsState] = JsonStringNew;
				break;
			case DtlsTransport::DtlsState::CONNECTING:
				json[JsonStringDtlsState] = JsonStringConnecting;
				break;
			case DtlsTransport::DtlsState::CONNECTED:
				json[JsonStringDtlsState] = JsonStringConnected;
				break;
			case DtlsTransport::DtlsState::FAILED:
				json[JsonStringDtlsState] = JsonStringFailed;
				break;
			case DtlsTransport::DtlsState::CLOSED:
				json[JsonStringDtlsState] = JsonStringClosed;
				break;
		}

		// Add headerExtensionIds.

		if (this->headerExtensionIds.absSendTime != 0u)
			jsonHeaderExtensionIds[JsonStringAbsSendTime] = this->headerExtensionIds.absSendTime;

		if (this->headerExtensionIds.rid != 0u)
			jsonHeaderExtensionIds[JsonStringRid] = this->headerExtensionIds.rid;

		json[JsonStringHeaderExtensionIds] = jsonHeaderExtensionIds;

		// Add maxBitrate.
		json[JsonStringMaxBitrate] = Json::UInt{ this->maxBitrate };

		// Add effectiveMaxBitrate.
		json[JsonStringEffectiveMaxBitrate] = Json::UInt{ this->effectiveMaxBitrate };

		// Add rtpListener.
		json[JsonStringRtpListener] = this->rtpListener.ToJson();

		return json;
	}

	RTC::DtlsTransport::Role WebRtcTransport::SetRemoteDtlsParameters(
	  RTC::DtlsTransport::Fingerprint& fingerprint, RTC::DtlsTransport::Role role)
	{
		MS_TRACE();

		// Ensure this method is not called twice.
		if (this->hasRemoteDtlsParameters)
			MS_THROW_ERROR("Transport already has remote DTLS parameters");

		if (fingerprint.algorithm == RTC::DtlsTransport::FingerprintAlgorithm::NONE)
			MS_THROW_ERROR("unsupported remote fingerprint algorithm");

		// Set local DTLS role.
		switch (role)
		{
			case RTC::DtlsTransport::Role::CLIENT:
			{
				this->dtlsLocalRole = RTC::DtlsTransport::Role::SERVER;

				break;
			}
			case RTC::DtlsTransport::Role::SERVER:
			{
				this->dtlsLocalRole = RTC::DtlsTransport::Role::CLIENT;

				break;
			}
			// If the peer has "auto" we become "client" since we are ICE controlled.
			case RTC::DtlsTransport::Role::AUTO:
			{
				this->dtlsLocalRole = RTC::DtlsTransport::Role::CLIENT;

				break;
			}
			case RTC::DtlsTransport::Role::NONE:
			{
				MS_THROW_ERROR("invalid remote role");
			}
		}

		this->hasRemoteDtlsParameters = true;

		// Pass the remote fingerprint to the DTLS transport.
		this->dtlsTransport->SetRemoteFingerprint(fingerprint);

		// Run the DTLS transport if ready.
		MayRunDtlsTransport();

		MS_DEBUG_DEV("Transport remote DTLS parameters set [transportId:%" PRIu32 "]", this->transportId);

		return this->dtlsLocalRole;
	}

	void WebRtcTransport::SetMaxBitrate(uint32_t bitrate)
	{
		MS_TRACE();

		static constexpr uint32_t MinBitrate{ 10000 };

		if (bitrate < MinBitrate)
			bitrate = MinBitrate;

		this->maxBitrate = bitrate;

		MS_DEBUG_TAG(rbe, "Transport max bitrate set to %" PRIu32 "bps", this->maxBitrate);
	}

	void WebRtcTransport::ChangeUfragPwd(std::string& usernameFragment, std::string& password)
	{
		MS_TRACE();

		this->iceServer->SetUsernameFragment(usernameFragment);
		this->iceServer->SetPassword(password);

		MS_DEBUG_DEV("Transport ICE ufrag&pwd changed [transportId:%" PRIu32 "]", this->transportId);
	}

	void WebRtcTransport::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Ensure there is sending SRTP session.
		if (this->srtpSendSession == nullptr)
		{
			MS_WARN_DEV("ignoring RTP packet due to non sending SRTP session");

			return;
		}

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		if (!this->srtpSendSession->EncryptRtp(&data, &len))
			return;

		this->selectedTuple->Send(data, len);
	}

	void WebRtcTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Ensure there is sending SRTP session.
		if (this->srtpSendSession == nullptr)
		{
			MS_WARN_DEV("ignoring RTCP packet due to non sending SRTP session");

			return;
		}

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		if (!this->srtpSendSession->EncryptRtcp(&data, &len))
			return;

		this->selectedTuple->Send(data, len);
	}

	bool WebRtcTransport::IsConnected() const
	{
		return (
		  this->selectedTuple != nullptr &&
		  this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTED);
	}

	inline void WebRtcTransport::MayRunDtlsTransport()
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
			case RTC::DtlsTransport::Role::AUTO:
				if (
				  this->iceServer->GetState() == RTC::IceServer::IceState::CONNECTED ||
				  this->iceServer->GetState() == RTC::IceServer::IceState::COMPLETED)
				{
					MS_DEBUG_TAG(
					  dtls, "transition from DTLS local role 'auto' to 'server' and running DTLS transport");

					this->dtlsLocalRole = RTC::DtlsTransport::Role::SERVER;
					this->dtlsTransport->Run(RTC::DtlsTransport::Role::SERVER);
				}
				break;

			// 'client' is only set if a 'setRemoteDtlsParameters' request was previously
			// received with remote DTLS role 'server'.
			// If 'client' then wait for ICE to be 'completed' (got USE-CANDIDATE).
			case RTC::DtlsTransport::Role::CLIENT:
				if (this->iceServer->GetState() == RTC::IceServer::IceState::COMPLETED)
				{
					MS_DEBUG_TAG(dtls, "running DTLS transport in local role 'client'");

					this->dtlsTransport->Run(RTC::DtlsTransport::Role::CLIENT);
				}
				break;

			// If 'server' then run the DTLS transport if ICE is 'connected' (not yet
			// USE-CANDIDATE) or 'completed'.
			case RTC::DtlsTransport::Role::SERVER:
				if (
				  this->iceServer->GetState() == RTC::IceServer::IceState::CONNECTED ||
				  this->iceServer->GetState() == RTC::IceServer::IceState::COMPLETED)
				{
					MS_DEBUG_TAG(dtls, "running DTLS transport in local role 'server'");

					this->dtlsTransport->Run(RTC::DtlsTransport::Role::SERVER);
				}
				break;

			case RTC::DtlsTransport::Role::NONE:
				MS_ABORT("local DTLS role not set");
		}
	}

	void WebRtcTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		// Ensure there is sending SRTP session.
		if (this->srtpSendSession == nullptr)
		{
			MS_WARN_DEV("ignoring RTCP packet due to non sending SRTP session");

			return;
		}

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		if (!this->srtpSendSession->EncryptRtcp(&data, &len))
			return;

		this->selectedTuple->Send(data, len);
	}

	inline void WebRtcTransport::OnPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Check if it's STUN.
		if (StunMessage::IsStun(data, len))
		{
			OnStunDataRecv(tuple, data, len);
		}
		// Check if it's RTCP.
		else if (RTCP::Packet::IsRtcp(data, len))
		{
			OnRtcpDataRecv(tuple, data, len);
		}
		// Check if it's RTP.
		else if (RtpPacket::IsRtp(data, len))
		{
			OnRtpDataRecv(tuple, data, len);
		}
		// Check if it's DTLS.
		else if (DtlsTransport::IsDtls(data, len))
		{
			OnDtlsDataRecv(tuple, data, len);
		}
		else
		{
			MS_WARN_DEV("ignoring received packet of unknown type");
		}
	}

	inline void WebRtcTransport::OnStunDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::StunMessage* msg = RTC::StunMessage::Parse(data, len);
		if (msg == nullptr)
		{
			MS_WARN_DEV("ignoring wrong STUN message received");

			return;
		}

		// Pass it to the IceServer.
		this->iceServer->ProcessStunMessage(msg, tuple);

		delete msg;
	}

	inline void WebRtcTransport::OnDtlsDataRecv(
	  const RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Ensure it comes from a valid tuple.
		if (!this->iceServer->IsValidTuple(tuple))
		{
			MS_WARN_TAG(dtls, "ignoring DTLS data coming from an invalid tuple");

			return;
		}

		// Trick for clients performing aggressive ICE regardless we are ICE-Lite.
		this->iceServer->ForceSelectedTuple(tuple);

		// Check that DTLS status is 'connecting' or 'connected'.
		if (
		  this->dtlsTransport->GetState() == DtlsTransport::DtlsState::CONNECTING ||
		  this->dtlsTransport->GetState() == DtlsTransport::DtlsState::CONNECTED)
		{
			MS_DEBUG_DEV("DTLS data received, passing it to the DTLS transport");

			this->dtlsTransport->ProcessDtlsData(data, len);
		}
		else
		{
			MS_WARN_TAG(dtls, "Transport is not 'connecting' or 'connected', ignoring received DTLS data");

			return;
		}
	}

	inline void WebRtcTransport::OnRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Ensure DTLS is connected.
		if (this->dtlsTransport->GetState() != RTC::DtlsTransport::DtlsState::CONNECTED)
		{
			MS_DEBUG_2TAGS(dtls, rtp, "ignoring RTP packet while DTLS not connected");

			return;
		}

		// Ensure there is receiving SRTP session.
		if (this->srtpRecvSession == nullptr)
		{
			MS_DEBUG_TAG(srtp, "ignoring RTP packet due to non receiving SRTP session");

			return;
		}

		// Ensure it comes from a valid tuple.
		if (!this->iceServer->IsValidTuple(tuple))
		{
			MS_WARN_TAG(rtp, "ignoring RTP packet coming from an invalid tuple");

			return;
		}

		// Decrypt the SRTP packet.
		if (!this->srtpRecvSession->DecryptSrtp(data, &len))
		{
			RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

			if (packet == nullptr)
			{
				MS_WARN_TAG(srtp, "DecryptSrtp() failed due to an invalid RTP packet");
			}
			else
			{
				MS_WARN_TAG(
				  srtp,
				  "DecryptSrtp() failed [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", seq:%" PRIu16 "]",
				  packet->GetSsrc(),
				  packet->GetPayloadType(),
				  packet->GetSequenceNumber());

				delete packet;
			}

			return;
		}

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Apply the Transport RTP header extension ids so the RTP listener can use them.

		if (this->headerExtensionIds.rid != 0u)
		{
			packet->AddExtensionMapping(
			  RtpHeaderExtensionUri::Type::RTP_STREAM_ID, this->headerExtensionIds.rid);
		}

		// Get the associated Producer.
		RTC::Producer* producer = this->rtpListener.GetProducer(packet);

		if (producer == nullptr)
		{
			MS_WARN_TAG(
			  rtp,
			  "no suitable Producer for received RTP packet [ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]",
			  packet->GetSsrc(),
			  packet->GetPayloadType());

			delete packet;
			return;
		}

		MS_DEBUG_DEV(
		  "RTP packet received [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", producer:%" PRIu32 "]",
		  packet->GetSsrc(),
		  packet->GetPayloadType(),
		  producer->producerId);

		// Trick for clients performing aggressive ICE regardless we are ICE-Lite.
		this->iceServer->ForceSelectedTuple(tuple);

		// Pass the RTP packet to the corresponding Producer.
		producer->ReceiveRtpPacket(packet);

		// Feed the remote bitrate estimator (REMB).
		uint32_t absSendTime;

		if (packet->ReadAbsSendTime(&absSendTime))
		{
			this->remoteBitrateEstimator->IncomingPacket(
			  DepLibUV::GetTime(), packet->GetPayloadLength(), *packet, absSendTime);
		}

		delete packet;
	}

	inline void WebRtcTransport::OnRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Ensure DTLS is connected.
		if (this->dtlsTransport->GetState() != RTC::DtlsTransport::DtlsState::CONNECTED)
		{
			MS_DEBUG_2TAGS(dtls, rtcp, "ignoring RTCP packet while DTLS not connected");

			return;
		}

		// Ensure there is receiving SRTP session.
		if (this->srtpRecvSession == nullptr)
		{
			MS_DEBUG_TAG(srtp, "ignoring RTCP packet due to non receiving SRTP session");

			return;
		}

		// Ensure it comes from a valid tuple.
		if (!this->iceServer->IsValidTuple(tuple))
		{
			MS_WARN_TAG(rtcp, "ignoring RTCP packet coming from an invalid tuple");

			return;
		}

		// Decrypt the SRTCP packet.
		if (!this->srtpRecvSession->DecryptSrtcp(data, &len))
			return;

		RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(data, len);

		if (packet == nullptr)
		{
			MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

			return;
		}

		// Handle each RTCP packet.
		while (packet != nullptr)
		{
			HandleRtcpPacket(packet);

			RTC::RTCP::Packet* previousPacket = packet;

			packet = packet->GetNext();
			delete previousPacket;
		}
	}

	void WebRtcTransport::OnPacketRecv(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketRecv(&tuple, data, len);
	}

	void WebRtcTransport::OnRtcTcpConnectionClosed(
	  RTC::TcpServer* /*tcpServer*/, RTC::TcpConnection* connection, bool isClosedByPeer)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		if (isClosedByPeer)
			this->iceServer->RemoveTuple(&tuple);
	}

	void WebRtcTransport::OnPacketRecv(RTC::TcpConnection* connection, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		OnPacketRecv(&tuple, data, len);
	}

	void WebRtcTransport::OnOutgoingStunMessage(
	  const RTC::IceServer* /*iceServer*/, const RTC::StunMessage* msg, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Send the STUN response over the same transport tuple.
		tuple->Send(msg->GetData(), msg->GetSize());
	}

	void WebRtcTransport::OnIceSelectedTuple(const RTC::IceServer* /*iceServer*/, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringIceSelectedTuple{ "iceSelectedTuple" };

		Json::Value eventData(Json::objectValue);

		/*
		 * RFC 5245 section 11.2 "Receiving Media":
		 *
		 * ICE implementations MUST be prepared to receive media on each component
		 * on any candidates provided for that component.
		 */

		// Update the selected tuple.
		this->selectedTuple = tuple;

		// Notify.
		eventData[JsonStringIceSelectedTuple] = tuple->ToJson();
		this->notifier->Emit(this->transportId, "iceselectedtuplechange", eventData);
	}

	void WebRtcTransport::OnIceConnected(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringIceState{ "iceState" };
		static const Json::StaticString JsonStringConnected{ "connected" };

		Json::Value eventData(Json::objectValue);

		MS_DEBUG_TAG(ice, "ICE connected");

		// Notify.
		eventData[JsonStringIceState] = JsonStringConnected;
		this->notifier->Emit(this->transportId, "icestatechange", eventData);

		// If ready, run the DTLS handler.
		MayRunDtlsTransport();
	}

	void WebRtcTransport::OnIceCompleted(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringIceState{ "iceState" };
		static const Json::StaticString JsonStringCompleted{ "completed" };

		Json::Value eventData(Json::objectValue);

		MS_DEBUG_TAG(ice, "ICE completed");

		// Notify.
		eventData[JsonStringIceState] = JsonStringCompleted;
		this->notifier->Emit(this->transportId, "icestatechange", eventData);

		// If ready, run the DTLS handler.
		MayRunDtlsTransport();
	}

	void WebRtcTransport::OnIceDisconnected(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringIceState{ "iceState" };
		static const Json::StaticString JsonStringDisconnected{ "disconnected" };

		Json::Value eventData(Json::objectValue);

		MS_DEBUG_TAG(ice, "ICE disconnected");

		// Unset the selected tuple.
		this->selectedTuple = nullptr;

		// Notify.
		eventData[JsonStringIceState] = JsonStringDisconnected;
		this->notifier->Emit(this->transportId, "icestatechange", eventData);

		// This is a fatal error so close the transport.
		Destroy();
	}

	void WebRtcTransport::OnDtlsConnecting(const RTC::DtlsTransport* /*dtlsTransport*/)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringDtlsState{ "dtlsState" };
		static const Json::StaticString JsonStringConnecting{ "connecting" };

		Json::Value eventData(Json::objectValue);

		MS_DEBUG_TAG(dtls, "DTLS connecting");

		// Notify.
		eventData[JsonStringDtlsState] = JsonStringConnecting;
		this->notifier->Emit(this->transportId, "dtlsstatechange", eventData);
	}

	void WebRtcTransport::OnDtlsConnected(
	  const RTC::DtlsTransport* /*dtlsTransport*/,
	  RTC::SrtpSession::Profile srtpProfile,
	  uint8_t* srtpLocalKey,
	  size_t srtpLocalKeyLen,
	  uint8_t* srtpRemoteKey,
	  size_t srtpRemoteKeyLen,
	  std::string& remoteCert)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringDtlsState{ "dtlsState" };
		static const Json::StaticString JsonStringConnected{ "connected" };
		static const Json::StaticString JsonStringDtlsRemoteCert{ "dtlsRemoteCert" };

		Json::Value eventData(Json::objectValue);

		MS_DEBUG_TAG(dtls, "DTLS connected");

		// Close it if it was already set and update it.
		if (this->srtpSendSession != nullptr)
		{
			this->srtpSendSession->Destroy();
			this->srtpSendSession = nullptr;
		}
		if (this->srtpRecvSession != nullptr)
		{
			this->srtpRecvSession->Destroy();
			this->srtpRecvSession = nullptr;
		}

		try
		{
			this->srtpSendSession = new RTC::SrtpSession(
			  RTC::SrtpSession::Type::OUTBOUND, srtpProfile, srtpLocalKey, srtpLocalKeyLen);
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR("error creating SRTP sending session: %s", error.what());
		}

		try
		{
			this->srtpRecvSession = new RTC::SrtpSession(
			  SrtpSession::Type::INBOUND, srtpProfile, srtpRemoteKey, srtpRemoteKeyLen);
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR("error creating SRTP receiving session: %s", error.what());

			this->srtpSendSession->Destroy();
			this->srtpSendSession = nullptr;
		}

		// Notify.
		eventData[JsonStringDtlsState]      = JsonStringConnected;
		eventData[JsonStringDtlsRemoteCert] = remoteCert;
		this->notifier->Emit(this->transportId, "dtlsstatechange", eventData);

		// Iterate all the Consumers and request key frame.
		for (auto* consumer : this->consumers)
		{
			if (consumer->kind == RTC::Media::Kind::VIDEO)
				MS_DEBUG_2TAGS(rtcp, rtx, "Transport connected, requesting key frame for Consumers");

			consumer->RequestKeyFrame();
		}
	}

	void WebRtcTransport::OnDtlsFailed(const RTC::DtlsTransport* /*dtlsTransport*/)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringDtlsState{ "dtlsState" };
		static const Json::StaticString JsonStringFailed{ "failed" };

		Json::Value eventData(Json::objectValue);

		MS_WARN_TAG(dtls, "DTLS failed");

		// Notify.
		eventData[JsonStringDtlsState] = JsonStringFailed;
		this->notifier->Emit(this->transportId, "dtlsstatechange", eventData);

		// This is a fatal error so close the transport.
		Destroy();
	}

	void WebRtcTransport::OnDtlsClosed(const RTC::DtlsTransport* /*dtlsTransport*/)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringDtlsState{ "dtlsState" };
		static const Json::StaticString JsonStringClosed{ "closed" };

		Json::Value eventData(Json::objectValue);

		MS_DEBUG_TAG(dtls, "DTLS remotely closed");

		// Notify.
		eventData[JsonStringDtlsState] = JsonStringClosed;
		this->notifier->Emit(this->transportId, "dtlsstatechange", eventData);

		// This is a fatal error so close the transport.
		Destroy();
	}

	void WebRtcTransport::OnOutgoingDtlsData(
	  const RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (this->selectedTuple == nullptr)
		{
			MS_WARN_TAG(dtls, "no selected tuple set, cannot send DTLS packet");

			return;
		}

		this->selectedTuple->Send(data, len);
	}

	void WebRtcTransport::OnDtlsApplicationData(
	  const RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* /*data*/, size_t len)
	{
		MS_TRACE();

		MS_DEBUG_TAG(dtls, "DTLS application data received [size:%zu]", len);

		// NOTE: No DataChannel support, si just ignore it.
	}

	void WebRtcTransport::OnRemoteBitrateEstimatorValue(const std::vector<uint32_t>& ssrcs, uint32_t bitrate)
	{
		MS_TRACE();

		uint32_t effectiveBitrate;
		uint64_t now = DepLibUV::GetTime();

		// Limit bitrate if requested via API.
		if (this->maxBitrate != 0u)
			effectiveBitrate = std::min(bitrate, this->maxBitrate);
		else
			effectiveBitrate = bitrate;

		if (MS_HAS_DEBUG_TAG(rbe))
		{
			std::ostringstream ssrcsStream;

			if (!ssrcs.empty())
			{
				std::copy(ssrcs.begin(), ssrcs.end() - 1, std::ostream_iterator<uint32_t>(ssrcsStream, ","));
				ssrcsStream << ssrcs.back();
			}

			MS_DEBUG_TAG(
			  rbe,
			  "sending RTCP REMB packet [estimated:%" PRIu32 "bps, effective:%" PRIu32 "bps, ssrcs:%s]",
			  bitrate,
			  effectiveBitrate,
			  ssrcsStream.str().c_str());
		}

		RTC::RTCP::FeedbackPsRembPacket packet(0, 0);

		packet.SetBitrate(effectiveBitrate);
		packet.SetSsrcs(ssrcs);
		packet.Serialize(RTC::RTCP::Buffer);
		this->SendRtcpPacket(&packet);

		if (now - this->lastEffectiveMaxBitrateAt > EffectiveMaxBitrateCheckInterval)
		{
			this->lastEffectiveMaxBitrateAt = now;
			this->effectiveMaxBitrate       = effectiveBitrate;
		}
	}
} // namespace RTC
