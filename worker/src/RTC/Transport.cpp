#define MS_CLASS "RTC::Transport"
// #define MS_LOG_DEV

#include "RTC/Transport.hpp"
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

/* Consts. */

static constexpr uint16_t IceCandidateDefaultLocalPriority{ 20000 };
static constexpr uint16_t IceCandidateLocalPriorityPreferFamilyIncrement{ 10000 };
static constexpr uint16_t IceCandidateLocalPriorityPreferProtocolIncrement{ 5000 };
// We just provide "host" candidates so type preference is fixed.
static constexpr uint16_t IceTypePreference{ 64 };
// We do not support non rtcp-mux so component is always 1.
static constexpr uint16_t IceComponent{ 1 };

/* Static helpers. */

static inline uint32_t generateIceCandidatePriority(uint16_t localPreference)
{
	MS_TRACE();

	return std::pow(2, 24) * IceTypePreference + std::pow(2, 8) * localPreference +
	       std::pow(2, 0) * (256 - IceComponent);
}

namespace RTC
{
	/* Static. */

	static constexpr uint64_t EffectiveMaxBitrateCheckInterval{ 2000 };        // In ms.
	static constexpr double EffectiveMaxBitrateThresholdBeforeKeyFrame{ 0.6 }; // 0.0 - 1.0.

	/* Instance methods. */

	Transport::Transport(
	  Listener* listener, Channel::Notifier* notifier, uint32_t transportId, TransportOptions& options)
	  : transportId(transportId), listener(listener), notifier(notifier)
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
				auto udpSocket = new RTC::UdpSocket(this, AF_INET);
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
				auto udpSocket = new RTC::UdpSocket(this, AF_INET6);
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
				auto tcpServer = new RTC::TcpServer(this, this, AF_INET);
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
				auto tcpServer = new RTC::TcpServer(this, this, AF_INET6);
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
			Destroy();

			MS_THROW_ERROR("could not open any IP:port");
		}

		// Create a DTLS agent.
		this->dtlsTransport = new RTC::DtlsTransport(this);

		// Create the RTCP timer.
		this->rtcpTimer = new Timer(this);

		// Start the RTCP timer.
		this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));

		// Hack to avoid that Destroy() above attempts to delete this.
		this->allocated = true;
	}

	Transport::~Transport()
	{
		MS_TRACE();

		// Destroy the RTCP timer.
		this->rtcpTimer->Destroy();
	}

	void Transport::Destroy()
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

		for (auto socket : this->udpSockets)
		{
			socket->Destroy();
		}
		this->udpSockets.clear();

		for (auto server : this->tcpServers)
		{
			server->Destroy();
		}
		this->tcpServers.clear();

		this->selectedTuple = nullptr;

		// Close all the handled Producers.
		for (auto it = this->producers.begin(); it != this->producers.end();)
		{
			auto* producer = *it;

			it = this->producers.erase(it);
			producer->Destroy();
		}

		// Disable all the handled Consumers.
		for (auto* consumer : this->consumers)
		{
			consumer->Disable();

			// Add us as listener.
			consumer->RemoveListener(this);
		}

		// TODO: yes? May be since we allow Transport being closed from on DtlsTransport
		// events...
		// Notify.
		this->notifier->Emit(this->transportId, "close");

		// If this was allocated (it did not throw in the constructor) notify the
		// listener and delete it.
		if (this->allocated)
		{
			// Notify the listener.
			this->listener->OnTransportClosed(this);

			delete this;
		}
	}

	Json::Value Transport::ToJson() const
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
		static const Json::StaticString JsonStringUseRemb{ "useRemb" };
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

		// Add useRemb.
		json[JsonStringUseRemb] = (static_cast<bool>(this->remoteBitrateEstimator));

		// Add maxBitrate.
		json[JsonStringMaxBitrate] = Json::UInt{ this->maxBitrate };

		// Add effectiveMaxBitrate.
		json[JsonStringEffectiveMaxBitrate] = Json::UInt{ this->effectiveMaxBitrate };

		// Add rtpListener.
		json[JsonStringRtpListener] = this->rtpListener.ToJson();

		return json;
	}

	void Transport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void Transport::HandleProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		// Pass it to the RtpListener.
		// NOTE: This may throw.
		this->rtpListener.AddProducer(producer);

		// Add to the map.
		this->producers.insert(producer);

		// Add us as listener.
		producer->AddListener(this);

		// Take the transport related RTP header extension ids of the Producer
		// and add them to the Transport.

		if (producer->GetTransportHeaderExtensionIds().absSendTime != 0u)
		{
			this->headerExtensionIds.absSendTime = producer->GetTransportHeaderExtensionIds().absSendTime;
		}

		if (producer->GetTransportHeaderExtensionIds().rid != 0u)
		{
			this->headerExtensionIds.rid = producer->GetTransportHeaderExtensionIds().rid;
		}
	}

	void Transport::HandleConsumer(RTC::Consumer* consumer)
	{
		MS_TRACE();

		// Add to the map.
		this->consumers.insert(consumer);

		// Add us as listener.
		consumer->AddListener(this);

		// If we are connected, ask a key request for this enabled Consumer.
		if (IsConnected())
		{
			if (consumer->kind == RTC::Media::Kind::VIDEO)
			{
				MS_DEBUG_TAG(
				  rtcp, "requesting key frame for new Consumer since Transport already connected");
			}

			consumer->RequestKeyFrame();
		}
	}

	RTC::DtlsTransport::Role Transport::SetRemoteDtlsParameters(
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

	void Transport::SetMaxBitrate(uint32_t bitrate)
	{
		MS_TRACE();

		static constexpr uint32_t MinBitrate{ 10000 };

		if (bitrate < MinBitrate)
			bitrate = MinBitrate;

		this->maxBitrate = bitrate;

		MS_DEBUG_TAG(rbe, "Transport max bitrate set to %" PRIu32 "bps", this->maxBitrate);
	}

	void Transport::ChangeUfragPwd(std::string& usernameFragment, std::string& password)
	{
		MS_TRACE();

		this->iceServer->SetUsernameFragment(usernameFragment);
		this->iceServer->SetPassword(password);

		MS_DEBUG_DEV("Transport ICE ufrag&pwd changed [transportId:%" PRIu32 "]", this->transportId);
	}

	void Transport::SendRtpPacket(RTC::RtpPacket* packet)
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

	void Transport::SendRtcpPacket(RTC::RTCP::Packet* packet)
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

	void Transport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
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

	inline void Transport::MayRunDtlsTransport()
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

	inline void Transport::HandleRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		switch (packet->GetType())
		{
			case RTCP::Type::RR:
			{
				auto* rr = dynamic_cast<RTCP::ReceiverReportPacket*>(packet);
				auto it  = rr->Begin();

				for (; it != rr->End(); ++it)
				{
					auto& report   = (*it);
					auto* consumer = GetConsumer(report->GetSsrc());

					if (consumer == nullptr)
					{
						MS_WARN_TAG(
						  rtcp,
						  "no Consumer found for received Receiver Report [ssrc:%" PRIu32 "]",
						  report->GetSsrc());

						break;
					}

					consumer->ReceiveRtcpReceiverReport(report);
				}

				break;
			}

			case RTCP::Type::PSFB:
			{
				auto* feedback = dynamic_cast<RTCP::FeedbackPsPacket*>(packet);

				switch (feedback->GetMessageType())
				{
					case RTCP::FeedbackPs::MessageType::PLI:
					case RTCP::FeedbackPs::MessageType::FIR:
					{
						auto* consumer = GetConsumer(feedback->GetMediaSsrc());

						if (consumer == nullptr)
						{
							MS_WARN_TAG(
							  rtcp,
							  "no Consumer found for received %s Feedback packet "
							  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
							  RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
							  feedback->GetMediaSsrc(),
							  feedback->GetMediaSsrc());

							break;
						}

						MS_DEBUG_TAG(
						  rtcp,
						  "%s received, requesting key frame for Consumer "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
						  feedback->GetMediaSsrc(),
						  feedback->GetMediaSsrc());

						consumer->RequestKeyFrame();

						break;
					}

					case RTCP::FeedbackPs::MessageType::AFB:
					{
						auto* afb = dynamic_cast<RTCP::FeedbackPsAfbPacket*>(feedback);

						// Ignore REMB requests.
						if (afb->GetApplication() == RTCP::FeedbackPsAfbPacket::Application::REMB)
							break;
					}

					// [[fallthrough]]; (C++17)
					case RTCP::FeedbackPs::MessageType::SLI:
					case RTCP::FeedbackPs::MessageType::RPSI:
					{
						auto* consumer = GetConsumer(feedback->GetMediaSsrc());

						if (consumer == nullptr)
						{
							MS_WARN_TAG(
							  rtcp,
							  "no Consumer found for received %s Feedback packet "
							  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
							  RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
							  feedback->GetMediaSsrc(),
							  feedback->GetMediaSsrc());

							break;
						}

						listener->OnTransportReceiveRtcpFeedback(this, consumer, feedback);

						break;
					}

					default:
					{
						MS_WARN_TAG(
						  rtcp,
						  "ignoring unsupported %s Feedback packet "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
						  feedback->GetMediaSsrc(),
						  feedback->GetMediaSsrc());

						break;
					}
				}

				break;
			}

			case RTCP::Type::RTPFB:
			{
				auto* feedback = dynamic_cast<RTCP::FeedbackRtpPacket*>(packet);
				auto* consumer = GetConsumer(feedback->GetMediaSsrc());

				if (consumer == nullptr)
				{
					MS_WARN_TAG(
					  rtcp,
					  "no Consumer found for received Feedback packet "
					  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
					  feedback->GetMediaSsrc(),
					  feedback->GetMediaSsrc());

					break;
				}

				switch (feedback->GetMessageType())
				{
					case RTCP::FeedbackRtp::MessageType::NACK:
					{
						auto* nackPacket = dynamic_cast<RTC::RTCP::FeedbackRtpNackPacket*>(packet);

						consumer->ReceiveNack(nackPacket);

						break;
					}

					default:
					{
						MS_WARN_TAG(
						  rtcp,
						  "ignoring unsupported %s Feedback packet "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  RTCP::FeedbackRtpPacket::MessageType2String(feedback->GetMessageType()).c_str(),
						  feedback->GetMediaSsrc(),
						  feedback->GetMediaSsrc());

						break;
					}
				}

				break;
			}

			case RTCP::Type::SR:
			{
				auto* sr = dynamic_cast<RTCP::SenderReportPacket*>(packet);
				auto it  = sr->Begin();

				// Even if Sender Report packet can only contain one report..
				for (; it != sr->End(); ++it)
				{
					auto& report = (*it);
					// Get the producer associated to the SSRC indicated in the report.
					auto* producer = this->rtpListener.GetProducer(report->GetSsrc());

					if (producer == nullptr)
					{
						MS_WARN_TAG(
						  rtcp,
						  "no Producer found for received Sender Report [ssrc:%" PRIu32 "]",
						  report->GetSsrc());

						continue;
					}

					producer->ReceiveRtcpSenderReport(report);
				}

				break;
			}

			case RTCP::Type::SDES:
			{
				auto* sdes = dynamic_cast<RTCP::SdesPacket*>(packet);
				auto it    = sdes->Begin();

				for (; it != sdes->End(); ++it)
				{
					auto& chunk = (*it);
					// Get the producer associated to the SSRC indicated in the report.
					auto* producer = this->rtpListener.GetProducer(chunk->GetSsrc());

					if (producer == nullptr)
					{
						MS_WARN_TAG(
						  rtcp, "no Producer for received SDES chunk [ssrc:%" PRIu32 "]", chunk->GetSsrc());

						continue;
					}

					// TODO: Should we do something with the SDES packet?
				}

				break;
			}

			case RTCP::Type::BYE:
			{
				MS_DEBUG_TAG(rtcp, "ignoring received RTCP BYE");

				break;
			}

			default:
			{
				MS_WARN_TAG(
				  rtcp,
				  "unhandled RTCP type received [type:%" PRIu8 "]",
				  static_cast<uint8_t>(packet->GetType()));
			}
		}
	}

	void Transport::SendRtcp(uint64_t now)
	{
		MS_TRACE();

		// - Create a CompoundPacket.
		// - Request every Consumer and Producer their RTCP data.
		// - Send the CompoundPacket.

		std::unique_ptr<RTC::RTCP::CompoundPacket> packet(new RTC::RTCP::CompoundPacket());

		for (auto& consumer : this->consumers)
		{
			consumer->GetRtcp(packet.get(), now);

			// Send the RTCP compound packet if there is a sender report.
			if (packet->GetSenderReportCount() != 0u)
			{
				// Ensure that the RTCP packet fits into the RTCP buffer.
				if (packet->GetSize() > RTC::RTCP::BufferSize)
				{
					MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

					return;
				}

				packet->Serialize(RTC::RTCP::Buffer);
				SendRtcpCompoundPacket(packet.get());
				// Reset the Compound packet.
				packet.reset(new RTC::RTCP::CompoundPacket());
			}
		}

		for (auto& producer : this->producers)
		{
			producer->GetRtcp(packet.get(), now);
		}

		// Send the RTCP compound with all receiver reports.
		if (packet->GetReceiverReportCount() != 0u)
		{
			// Ensure that the RTCP packet fits into the RTCP buffer.
			if (packet->GetSize() > RTC::RTCP::BufferSize)
			{
				MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

				return;
			}

			packet->Serialize(RTC::RTCP::Buffer);
			SendRtcpCompoundPacket(packet.get());
		}
	}

	inline RTC::Consumer* Transport::GetConsumer(uint32_t ssrc) const
	{
		MS_TRACE();

		for (auto* consumer : this->consumers)
		{
			// Ignore if not enabled.
			if (!consumer->IsEnabled())
				continue;

			// NOTE: Use & since, otherwise, a full copy will be retrieved.
			auto& rtpParameters = consumer->GetParameters();

			for (auto& encoding : rtpParameters.encodings)
			{
				if (encoding.ssrc == ssrc)
					return consumer;
				if (encoding.hasFec && encoding.fec.ssrc == ssrc)
					return consumer;
				if (encoding.hasRtx && encoding.rtx.ssrc == ssrc)
					return consumer;
			}
		}

		return nullptr;
	}

	inline void Transport::OnPacketRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
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

	inline void Transport::OnStunDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
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

	inline void Transport::OnDtlsDataRecv(const RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
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

	inline void Transport::OnRtpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
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

		// TODO: test getting RID.

		// const uint8_t* rid;
		// size_t ridLen;

		// if (packet->ReadRid(&rid, &ridLen))
		// {
		// 	std::string ridStr((const char*)rid, ridLen);

		// 	MS_ERROR("---------- HABEMUS RID !!!: %s", ridStr.c_str());
		// }

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
		if (this->remoteBitrateEstimator)
		{
			uint32_t absSendTime;

			if (packet->ReadAbsSendTime(&absSendTime))
			{
				this->remoteBitrateEstimator->IncomingPacket(
				  DepLibUV::GetTime(), packet->GetPayloadLength(), *packet, absSendTime);
			}
		}

		delete packet;
	}

	inline void Transport::OnRtcpDataRecv(RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
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

	void Transport::OnPacketRecv(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketRecv(&tuple, data, len);
	}

	void Transport::OnRtcTcpConnectionClosed(
	  RTC::TcpServer* /*tcpServer*/, RTC::TcpConnection* connection, bool isClosedByPeer)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		if (isClosedByPeer)
			this->iceServer->RemoveTuple(&tuple);
	}

	void Transport::OnPacketRecv(RTC::TcpConnection* connection, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		OnPacketRecv(&tuple, data, len);
	}

	void Transport::OnOutgoingStunMessage(
	  const RTC::IceServer* /*iceServer*/, const RTC::StunMessage* msg, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Send the STUN response over the same transport tuple.
		tuple->Send(msg->GetData(), msg->GetSize());
	}

	void Transport::OnIceSelectedTuple(const RTC::IceServer* /*iceServer*/, RTC::TransportTuple* tuple)
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

	void Transport::OnIceConnected(const RTC::IceServer* /*iceServer*/)
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

	void Transport::OnIceCompleted(const RTC::IceServer* /*iceServer*/)
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

	void Transport::OnIceDisconnected(const RTC::IceServer* /*iceServer*/)
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

	void Transport::OnDtlsConnecting(const RTC::DtlsTransport* /*dtlsTransport*/)
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

	void Transport::OnDtlsConnected(
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
				MS_DEBUG_TAG(rtcp, "Transport connected, requesting key frame for Consumers");

			consumer->RequestKeyFrame();
		}
	}

	void Transport::OnDtlsFailed(const RTC::DtlsTransport* /*dtlsTransport*/)
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

	void Transport::OnDtlsClosed(const RTC::DtlsTransport* /*dtlsTransport*/)
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

	void Transport::OnOutgoingDtlsData(
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

	void Transport::OnDtlsApplicationData(
	  const RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* /*data*/, size_t len)
	{
		MS_TRACE();

		MS_DEBUG_TAG(dtls, "DTLS application data received [size:%zu]", len);

		// NOTE: No DataChannel support, si just ignore it.
	}

	void Transport::OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs, uint32_t bitrate)
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

		// Trigger a key frame for all the suitable streams if the effective max bitrate
		// has decreased abruptly.
		if (now - this->lastEffectiveMaxBitrateAt > EffectiveMaxBitrateCheckInterval)
		{
			if (
			  (bitrate != 0u) && (this->effectiveMaxBitrate != 0u) &&
			  static_cast<double>(effectiveBitrate) / static_cast<double>(this->effectiveMaxBitrate) <
			    EffectiveMaxBitrateThresholdBeforeKeyFrame)
			{
				MS_WARN_TAG(rbe, "uplink effective max bitrate abruptly decreased, requesting key frames");

				// Request key frame for all the Producers.
				for (auto* producer : this->producers)
				{
					producer->RequestKeyFrame();
				}
			}

			this->lastEffectiveMaxBitrateAt = now;
			this->effectiveMaxBitrate       = effectiveBitrate;
		}
	}

	void Transport::OnProducerClosed(RTC::Producer* producer)
	{
		MS_TRACE();

		// Remove it from the map.
		this->producers.erase(producer);

		// Remove it from the RtpListener.
		this->rtpListener.RemoveProducer(producer);
	}

	void Transport::OnProducerRtpParametersUpdated(RTC::Producer* producer)
	{
		MS_TRACE();

		// Update our RtpListener.
		// NOTE: This may throw.
		this->rtpListener.AddProducer(producer);
	}

	void Transport::OnProducerPaused(RTC::Producer* /*producer*/)
	{
		// Do nothing.
	}

	void Transport::OnProducerResumed(RTC::Producer* /*producer*/)
	{
		// Do nothing.
	}

	void Transport::OnProducerRtpPacket(
	  RTC::Producer* /*producer*/,
	  RTC::RtpPacket* /*packet*/,
	  RTC::RtpEncodingParameters::Profile /*profile*/)
	{
		// Do nothing.
	}

	void Transport::OnProducerProfileEnabled(
	  RTC::Producer* /*producer*/, RTC::RtpEncodingParameters::Profile /*profile*/)
	{
		// Do nothing.
	}

	void Transport::OnProducerProfileDisabled(
	  RTC::Producer* /*producer*/, RTC::RtpEncodingParameters::Profile /*profile*/)
	{
		// Do nothing.
	}

	void Transport::OnConsumerClosed(RTC::Consumer* consumer)
	{
		MS_TRACE();

		// Remove from the map.
		this->consumers.erase(consumer);
	}

	void Transport::OnConsumerKeyFrameRequired(RTC::Consumer* /*consumer*/)
	{
		// Do nothing.
	}

	void Transport::OnTimer(Timer* timer)
	{
		if (timer == this->rtcpTimer)
		{
			uint64_t interval = RTC::RTCP::MaxVideoIntervalMs;
			uint32_t now      = DepLibUV::GetTime();

			SendRtcp(now);

			// Recalculate next RTCP interval.
			if (!this->consumers.empty())
			{
				// Transmission rate in kbps.
				uint32_t rate = 0;

				// Get the RTP sending rate.
				for (auto& consumer : this->consumers)
				{
					rate += consumer->GetTransmissionRate(now) / 1000;
				}

				// Calculate bandwidth: 360 / transmission bandwidth in kbit/s
				if (rate != 0u)
					interval = 360000 / rate;

				if (interval > RTC::RTCP::MaxVideoIntervalMs)
					interval = RTC::RTCP::MaxVideoIntervalMs;
			}

			/*
			 * The interval between RTCP packets is varied randomly over the range
			 * [0.5,1.5] times the calculated interval to avoid unintended synchronization
			 * of all participants.
			 */
			interval *= static_cast<float>(Utils::Crypto::GetRandomUInt(5, 15)) / 10;
			this->rtcpTimer->Start(interval);
		}
	}
} // namespace RTC
