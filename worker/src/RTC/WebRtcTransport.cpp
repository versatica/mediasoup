#define MS_CLASS "RTC::WebRtcTransport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/WebRtcTransport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cmath> // std::pow()

namespace RTC
{
	/* Static. */

	static constexpr uint16_t IceCandidateDefaultLocalPriority{ 10000 };
	// We just provide "host" candidates so type preference is fixed.
	static constexpr uint16_t IceTypePreference{ 64 };
	// We do not support non rtcp-mux so component is always 1.
	static constexpr uint16_t IceComponent{ 1 };

	static inline uint32_t generateIceCandidatePriority(uint16_t localPreference)
	{
		MS_TRACE();

		return std::pow(2, 24) * IceTypePreference + std::pow(2, 8) * localPreference +
		       std::pow(2, 0) * (256 - IceComponent);
	}

	/* Instance methods. */

	WebRtcTransport::WebRtcTransport(
	  RTC::Shared* shared,
	  const std::string& id,
	  RTC::Transport::Listener* listener,
	  const FBS::WebRtcTransport::WebRtcTransportOptions* options)
	  : RTC::Transport::Transport(shared, id, listener, options->base())
	{
		MS_TRACE();

		auto* listenInfo = options->listen_as<FBS::WebRtcTransport::WebRtcTransportListenIndividual>();
		auto* listenIps  = listenInfo->listenIps();

		try
		{
			uint16_t iceLocalPreferenceDecrement{ 0 };

			if (options->enableUdp() && options->enableTcp())
				this->iceCandidates.reserve(2 * listenIps->size());
			else
				this->iceCandidates.reserve(listenIps->size());

			for (const auto* listenIp : *listenIps)
			{
				auto ip = listenIp->ip()->str();

				// This may throw.
				Utils::IP::NormalizeIp(ip);

				if (options->enableUdp())
				{
					uint16_t iceLocalPreference =
					  IceCandidateDefaultLocalPriority - iceLocalPreferenceDecrement;

					if (options->preferUdp())
						iceLocalPreference += 1000;

					uint32_t icePriority = generateIceCandidatePriority(iceLocalPreference);

					// This may throw.
					RTC::UdpSocket* udpSocket;
					if (listenInfo->port() != 0)
						udpSocket = new RTC::UdpSocket(this, ip, listenInfo->port());
					else
						udpSocket = new RTC::UdpSocket(this, ip);

					auto announcedIp = listenIp->announcedIp()->str();

					this->udpSockets[udpSocket] = announcedIp;

					if (listenIp->announcedIp()->size() == 0)
						this->iceCandidates.emplace_back(udpSocket, icePriority);
					else
						this->iceCandidates.emplace_back(udpSocket, icePriority, announcedIp);
				}

				if (options->enableTcp())
				{
					uint16_t iceLocalPreference =
					  IceCandidateDefaultLocalPriority - iceLocalPreferenceDecrement;

					if (options->preferTcp())
						iceLocalPreference += 1000;

					uint32_t icePriority = generateIceCandidatePriority(iceLocalPreference);

					// This may throw.
					RTC::TcpServer* tcpServer;
					if (listenInfo->port() != 0)
						tcpServer = new RTC::TcpServer(this, this, ip, listenInfo->port());
					else
						tcpServer = new RTC::TcpServer(this, this, ip);

					auto announcedIp = listenIp->announcedIp()->str();

					this->tcpServers[tcpServer] = announcedIp;

					if (listenIp->announcedIp()->size() == 0)
						this->iceCandidates.emplace_back(tcpServer, icePriority);
					else
						this->iceCandidates.emplace_back(tcpServer, icePriority, announcedIp);
				}

				// Decrement initial ICE local preference for next IP.
				iceLocalPreferenceDecrement += 100;
			}

			// Create a ICE server.
			this->iceServer = new RTC::IceServer(
			  this, Utils::Crypto::GetRandomString(32), Utils::Crypto::GetRandomString(32));

			// Create a DTLS transport.
			this->dtlsTransport = new RTC::DtlsTransport(this);

			// NOTE: This may throw.
			this->shared->channelMessageRegistrator->RegisterHandler(
			  this->id,
			  /*channelRequestHandler*/ this,
			  /*payloadChannelRequestHandler*/ this,
			  /*payloadChannelNotificationHandler*/ this);
		}
		catch (const MediaSoupError& error)
		{
			// Must delete everything since the destructor won't be called.

			delete this->dtlsTransport;
			this->dtlsTransport = nullptr;

			delete this->iceServer;
			this->iceServer = nullptr;

			for (auto& kv : this->udpSockets)
			{
				auto* udpSocket = kv.first;

				delete udpSocket;
			}
			this->udpSockets.clear();

			for (auto& kv : this->tcpServers)
			{
				auto* tcpServer = kv.first;

				delete tcpServer;
			}
			this->tcpServers.clear();

			this->iceCandidates.clear();

			throw;
		}
	}

	/**
	 * This constructor is used when the WebRtcTransport uses a WebRtcServer.
	 */
	WebRtcTransport::WebRtcTransport(
	  RTC::Shared* shared,
	  const std::string& id,
	  RTC::Transport::Listener* listener,
	  WebRtcTransportListener* webRtcTransportListener,
	  std::vector<RTC::IceCandidate>& iceCandidates,
	  const FBS::WebRtcTransport::WebRtcTransportOptions* options)
	  : RTC::Transport::Transport(shared, id, listener, options->base()),
	    webRtcTransportListener(webRtcTransportListener), iceCandidates(iceCandidates)
	{
		MS_TRACE();

		try
		{
			if (iceCandidates.empty())
				MS_THROW_TYPE_ERROR("empty iceCandidates");

			// Create a ICE server.
			this->iceServer = new RTC::IceServer(
			  this, Utils::Crypto::GetRandomString(32), Utils::Crypto::GetRandomString(32));

			// Create a DTLS transport.
			this->dtlsTransport = new RTC::DtlsTransport(this);

			// Notify the webRtcTransportListener.
			this->webRtcTransportListener->OnWebRtcTransportCreated(this);

			// NOTE: This may throw.
			this->shared->channelMessageRegistrator->RegisterHandler(
			  this->id,
			  /*channelRequestHandler*/ this,
			  /*payloadChannelRequestHandler*/ this,
			  /*payloadChannelNotificationHandler*/ this);
		}
		catch (const MediaSoupError& error)
		{
			// Must delete everything since the destructor won't be called.

			delete this->dtlsTransport;
			this->dtlsTransport = nullptr;

			delete this->iceServer;
			this->iceServer = nullptr;

			this->iceCandidates.clear();

			throw;
		}
	}

	WebRtcTransport::~WebRtcTransport()
	{
		MS_TRACE();

		this->shared->channelMessageRegistrator->UnregisterHandler(this->id);

		// Must delete the DTLS transport first since it will generate a DTLS alert
		// to be sent.
		delete this->dtlsTransport;
		this->dtlsTransport = nullptr;

		delete this->iceServer;
		this->iceServer = nullptr;

		for (auto& kv : this->udpSockets)
		{
			auto* udpSocket = kv.first;

			delete udpSocket;
		}
		this->udpSockets.clear();

		for (auto& kv : this->tcpServers)
		{
			auto* tcpServer = kv.first;

			delete tcpServer;
		}
		this->tcpServers.clear();

		this->iceCandidates.clear();

		delete this->srtpSendSession;
		this->srtpSendSession = nullptr;

		delete this->srtpRecvSession;
		this->srtpRecvSession = nullptr;

		// Notify the webRtcTransportListener.
		if (this->webRtcTransportListener)
			this->webRtcTransportListener->OnWebRtcTransportClosed(this);
	}

	flatbuffers::Offset<FBS::Transport::DumpResponse> WebRtcTransport::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		// Add iceParameters.
		auto iceParameters = FBS::Transport::CreateIceParametersDirect(
		  builder,
		  this->iceServer->GetUsernameFragment().c_str(),
		  this->iceServer->GetPassword().c_str(),
		  true);

		std::vector<flatbuffers::Offset<FBS::Transport::IceCandidate>> iceCandidates;

		for (const auto& iceCandidate : this->iceCandidates)
		{
			iceCandidates.emplace_back(iceCandidate.FillBuffer(builder));
		}

		// Add iceState.
		std::string iceState;

		switch (this->iceServer->GetState())
		{
			case RTC::IceServer::IceState::NEW:
				iceState = "new";
				break;
			case RTC::IceServer::IceState::CONNECTED:
				iceState = "connected";
				break;
			case RTC::IceServer::IceState::COMPLETED:
				iceState = "completed";
				break;
			case RTC::IceServer::IceState::DISCONNECTED:
				iceState = "disconnected";
				break;
		}

		// Add iceSelectedTuple.
		flatbuffers::Offset<FBS::Transport::Tuple> iceSelectedTuple;

		if (this->iceServer->GetSelectedTuple())
			iceSelectedTuple = this->iceServer->GetSelectedTuple()->FillBuffer(builder);

		// Add dtlsParameters.fingerprints.
		std::vector<flatbuffers::Offset<FBS::Transport::Fingerprint>> fingerprints;

		for (const auto& fingerprint : this->dtlsTransport->GetLocalFingerprints())
		{
			auto& algorithm = RTC::DtlsTransport::GetFingerprintAlgorithmString(fingerprint.algorithm);
			auto& value     = fingerprint.value;

			fingerprints.emplace_back(
			  FBS::Transport::CreateFingerprintDirect(builder, algorithm.c_str(), value.c_str()));
		}

		// Add dtlsParameters.role.
		std::string dtlsRole;

		switch (this->dtlsRole)
		{
			case RTC::DtlsTransport::Role::NONE:
				dtlsRole = "none";
				break;
			case RTC::DtlsTransport::Role::AUTO:
				dtlsRole = "auto";
				break;
			case RTC::DtlsTransport::Role::CLIENT:
				dtlsRole = "client";
				break;
			case RTC::DtlsTransport::Role::SERVER:
				dtlsRole = "server";
				break;
		}

		// Add dtlsState.
		std::string dtlsState;

		switch (this->dtlsTransport->GetState())
		{
			case RTC::DtlsTransport::DtlsState::NEW:
				dtlsState = "new";
				break;
			case RTC::DtlsTransport::DtlsState::CONNECTING:
				dtlsState = "connecting";
				break;
			case RTC::DtlsTransport::DtlsState::CONNECTED:
				dtlsState = "connected";
				break;
			case RTC::DtlsTransport::DtlsState::FAILED:
				dtlsState = "failed";
				break;
			case RTC::DtlsTransport::DtlsState::CLOSED:
				dtlsState = "closed";
				break;
		}

		// Add base transport dump.
		auto base = Transport::FillBuffer(builder);
		// Add dtlsParameters.
		auto dtlsParameters =
		  FBS::Transport::CreateDtlsParametersDirect(builder, &fingerprints, dtlsRole.c_str());

		auto webRtcTransportDump = FBS::Transport::CreateWebRtcTransportDumpDirect(
		  builder,
		  base,
		  // iceRole (we are always "controlled").
		  "controlled",
		  iceParameters,
		  &iceCandidates,
		  iceState.c_str(),
		  iceSelectedTuple,
		  dtlsParameters,
		  dtlsState.c_str());

		return FBS::Transport::CreateDumpResponse(
		  builder, FBS::Transport::TransportDumpData::WebRtcTransportDump, webRtcTransportDump.Union());
	}

	void WebRtcTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJsonStats(jsonArray);

		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "webrtc-transport";

		// Add iceRole (we are always "controlled").
		jsonObject["iceRole"] = "controlled";

		// Add iceState.
		switch (this->iceServer->GetState())
		{
			case RTC::IceServer::IceState::NEW:
				jsonObject["iceState"] = "new";
				break;
			case RTC::IceServer::IceState::CONNECTED:
				jsonObject["iceState"] = "connected";
				break;
			case RTC::IceServer::IceState::COMPLETED:
				jsonObject["iceState"] = "completed";
				break;
			case RTC::IceServer::IceState::DISCONNECTED:
				jsonObject["iceState"] = "disconnected";
				break;
		}

		if (this->iceServer->GetSelectedTuple())
		{
			// Add iceSelectedTuple.
			this->iceServer->GetSelectedTuple()->FillJson(jsonObject["iceSelectedTuple"]);
		}

		// Add dtlsState.
		switch (this->dtlsTransport->GetState())
		{
			case RTC::DtlsTransport::DtlsState::NEW:
				jsonObject["dtlsState"] = "new";
				break;
			case RTC::DtlsTransport::DtlsState::CONNECTING:
				jsonObject["dtlsState"] = "connecting";
				break;
			case RTC::DtlsTransport::DtlsState::CONNECTED:
				jsonObject["dtlsState"] = "connected";
				break;
			case RTC::DtlsTransport::DtlsState::FAILED:
				jsonObject["dtlsState"] = "failed";
				break;
			case RTC::DtlsTransport::DtlsState::CLOSED:
				jsonObject["dtlsState"] = "closed";
				break;
		}
	}

	void WebRtcTransport::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->method)
		{
			case Channel::ChannelRequest::Method::TRANSPORT_CONNECT:
			{
				// Ensure this method is not called twice.
				if (this->connectCalled)
					MS_THROW_ERROR("connect() already called");

				auto body           = request->data->body_as<FBS::Transport::ConnectRequest>();
				auto connectData    = body->data_as<FBS::Transport::ConnectWebRtcTransportData>();
				auto dtlsParameters = connectData->dtlsParameters();

				RTC::DtlsTransport::Fingerprint dtlsRemoteFingerprint;
				RTC::DtlsTransport::Role dtlsRemoteRole;

				if (dtlsParameters->fingerprints()->size() == 0)
				{
					MS_THROW_TYPE_ERROR("empty dtlsParameters.fingerprints array");
				}

				// NOTE: Just take the first fingerprint.
				for (const auto& fingerprint : *dtlsParameters->fingerprints())
				{
					dtlsRemoteFingerprint.algorithm =
					  RTC::DtlsTransport::GetFingerprintAlgorithm(fingerprint->algorithm()->str());

					if (dtlsRemoteFingerprint.algorithm == RTC::DtlsTransport::FingerprintAlgorithm::NONE)
					{
						MS_THROW_TYPE_ERROR("invalid fingerprint.algorithm value");
					}

					dtlsRemoteFingerprint.value = fingerprint->value()->str();

					// Just use the first fingerprint.
					break;
				}

				if (flatbuffers::IsFieldPresent(dtlsParameters, FBS::Transport::DtlsParameters::VT_ROLE))
				{
					dtlsRemoteRole = RTC::DtlsTransport::StringToRole(dtlsParameters->role()->str());

					if (dtlsRemoteRole == RTC::DtlsTransport::Role::NONE)
						MS_THROW_TYPE_ERROR("invalid dtlsParameters.role value");
				}
				else
				{
					dtlsRemoteRole = RTC::DtlsTransport::Role::AUTO;
				}

				// Set local DTLS role.
				switch (dtlsRemoteRole)
				{
					case RTC::DtlsTransport::Role::CLIENT:
					{
						this->dtlsRole = RTC::DtlsTransport::Role::SERVER;

						break;
					}
					// If the peer has role "auto" we become "client" since we are ICE controlled.
					case RTC::DtlsTransport::Role::SERVER:
					case RTC::DtlsTransport::Role::AUTO:
					{
						this->dtlsRole = RTC::DtlsTransport::Role::CLIENT;

						break;
					}
					case RTC::DtlsTransport::Role::NONE:
					{
						MS_THROW_TYPE_ERROR("invalid remote DTLS role");
					}
				}

				this->connectCalled = true;

				// Pass the remote fingerprint to the DTLS transport.
				if (this->dtlsTransport->SetRemoteFingerprint(dtlsRemoteFingerprint))
				{
					// If everything is fine, we may run the DTLS transport if ready.
					MayRunDtlsTransport();
				}

				// Tell the caller about the selected local DTLS role.
				std::string dtlsLocalRole;

				switch (this->dtlsRole)
				{
					case RTC::DtlsTransport::Role::CLIENT:
						dtlsLocalRole = "client";
						break;

					case RTC::DtlsTransport::Role::SERVER:
						dtlsLocalRole = "server";
						break;

					default:
						MS_ABORT("invalid local DTLS role");
				}

				auto connectResponseDataOffset = FBS::Transport::CreateConnectWebRtcTransportResponseDirect(
				  request->GetBufferBuilder(), dtlsLocalRole.c_str());

				auto responseOffset = FBS::Transport::CreateConnectResponse(
				  request->GetBufferBuilder(),
				  FBS::Transport::ConnectResponseData::ConnectWebRtcTransportResponse,
				  connectResponseDataOffset.Union());

				request->Accept(FBS::Response::Body::FBS_Transport_ConnectResponse, responseOffset);

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_RESTART_ICE:
			{
				std::string usernameFragment = Utils::Crypto::GetRandomString(32);
				std::string password         = Utils::Crypto::GetRandomString(32);

				this->iceServer->RestartIce(usernameFragment, password);

				MS_DEBUG_DEV(
				  "WebRtcTransport ICE usernameFragment and password changed [id:%s]", this->id.c_str());

				// Reply with the updated ICE local parameters.
				auto responseOffset = FBS::Transport::CreateRestartIceResponseDirect(
				  request->GetBufferBuilder(),
				  this->iceServer->GetUsernameFragment().c_str(),
				  this->iceServer->GetPassword().c_str(),
				  true /* iceLite */
				);

				request->Accept(FBS::Response::Body::FBS_Transport_DumpResponse, responseOffset);

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Transport::HandleRequest(request);
			}
		}
	}

	void WebRtcTransport::HandleNotification(PayloadChannel::PayloadChannelNotification* notification)
	{
		MS_TRACE();

		// Pass it to the parent class.
		RTC::Transport::HandleNotification(notification);
	}

	void WebRtcTransport::ProcessStunPacketFromWebRtcServer(
	  RTC::TransportTuple* tuple, RTC::StunPacket* packet)
	{
		MS_TRACE();

		// Pass it to the IceServer.
		this->iceServer->ProcessStunPacket(packet, tuple);
	}

	void WebRtcTransport::ProcessNonStunPacketFromWebRtcServer(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Increase receive transmission.
		RTC::Transport::DataReceived(len);

		// Check if it's RTCP.
		if (RTC::RTCP::Packet::IsRtcp(data, len))
		{
			OnRtcpDataReceived(tuple, data, len);
		}
		// Check if it's RTP.
		else if (RTC::RtpPacket::IsRtp(data, len))
		{
			OnRtpDataReceived(tuple, data, len);
		}
		// Check if it's DTLS.
		else if (RTC::DtlsTransport::IsDtls(data, len))
		{
			OnDtlsDataReceived(tuple, data, len);
		}
		else
		{
			MS_WARN_DEV("ignoring received packet of unknown type");
		}
	}

	void WebRtcTransport::RemoveTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		this->iceServer->RemoveTuple(tuple);
	}

	inline bool WebRtcTransport::IsConnected() const
	{
		MS_TRACE();

		// clang-format off
		return (
			(
				this->iceServer->GetState() == RTC::IceServer::IceState::CONNECTED ||
				this->iceServer->GetState() == RTC::IceServer::IceState::COMPLETED
			) &&
			this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTED
		);
		// clang-format on
	}

	void WebRtcTransport::MayRunDtlsTransport()
	{
		MS_TRACE();

		// Do nothing if we have the same local DTLS role as the DTLS transport.
		// NOTE: local role in DTLS transport can be NONE, but not ours.
		if (this->dtlsTransport->GetLocalRole() == this->dtlsRole)
			return;

		// Check our local DTLS role.
		switch (this->dtlsRole)
		{
			// If still 'auto' then transition to 'server' if ICE is 'connected' or
			// 'completed'.
			case RTC::DtlsTransport::Role::AUTO:
			{
				// clang-format off
				if (
					this->iceServer->GetState() == RTC::IceServer::IceState::CONNECTED ||
					this->iceServer->GetState() == RTC::IceServer::IceState::COMPLETED
				)
				// clang-format on
				{
					MS_DEBUG_TAG(
					  dtls, "transition from DTLS local role 'auto' to 'server' and running DTLS transport");

					this->dtlsRole = RTC::DtlsTransport::Role::SERVER;
					this->dtlsTransport->Run(RTC::DtlsTransport::Role::SERVER);
				}

				break;
			}

			// 'client' is only set if a 'connect' request was previously called with
			// remote DTLS role 'server'.
			//
			// If 'client' then wait for ICE to be 'completed' (got USE-CANDIDATE).
			//
			// NOTE: This is the theory, however let's be more flexible as told here:
			//   https://bugs.chromium.org/p/webrtc/issues/detail?id=3661
			case RTC::DtlsTransport::Role::CLIENT:
			{
				// clang-format off
				if (
					this->iceServer->GetState() == RTC::IceServer::IceState::CONNECTED ||
					this->iceServer->GetState() == RTC::IceServer::IceState::COMPLETED
				)
				// clang-format on
				{
					MS_DEBUG_TAG(dtls, "running DTLS transport in local role 'client'");

					this->dtlsTransport->Run(RTC::DtlsTransport::Role::CLIENT);
				}

				break;
			}

			// If 'server' then run the DTLS transport if ICE is 'connected' (not yet
			// USE-CANDIDATE) or 'completed'.
			case RTC::DtlsTransport::Role::SERVER:
			{
				// clang-format off
				if (
					this->iceServer->GetState() == RTC::IceServer::IceState::CONNECTED ||
					this->iceServer->GetState() == RTC::IceServer::IceState::COMPLETED
				)
				// clang-format on
				{
					MS_DEBUG_TAG(dtls, "running DTLS transport in local role 'server'");

					this->dtlsTransport->Run(RTC::DtlsTransport::Role::SERVER);
				}

				break;
			}

			case RTC::DtlsTransport::Role::NONE:
			{
				MS_ABORT("local DTLS role not set");
			}
		}
	}

	void WebRtcTransport::SendRtpPacket(
	  RTC::Consumer* /*consumer*/, RTC::RtpPacket* packet, RTC::Transport::onSendCallback* cb)
	{
		MS_TRACE();

		if (!IsConnected())
		{
			if (cb)
			{
				(*cb)(false);
				delete cb;
			}

			return;
		}

		// Ensure there is sending SRTP session.
		if (!this->srtpSendSession)
		{
			MS_WARN_DEV("ignoring RTP packet due to non sending SRTP session");

			if (cb)
			{
				(*cb)(false);
				delete cb;
			}

			return;
		}

		const uint8_t* data = packet->GetData();
		auto intLen         = static_cast<int>(packet->GetSize());

		if (!this->srtpSendSession->EncryptRtp(&data, &intLen))
		{
			if (cb)
			{
				(*cb)(false);
				delete cb;
			}

			return;
		}

		auto len = static_cast<size_t>(intLen);

		this->iceServer->GetSelectedTuple()->Send(data, len, cb);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void WebRtcTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		auto intLen         = static_cast<int>(packet->GetSize());

		// Ensure there is sending SRTP session.
		if (!this->srtpSendSession)
		{
			MS_WARN_DEV("ignoring RTCP packet due to non sending SRTP session");

			return;
		}

		if (!this->srtpSendSession->EncryptRtcp(&data, &intLen))
			return;

		auto len = static_cast<size_t>(intLen);

		this->iceServer->GetSelectedTuple()->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void WebRtcTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		packet->Serialize(RTC::RTCP::Buffer);

		const uint8_t* data = packet->GetData();
		auto intLen         = static_cast<int>(packet->GetSize());

		// Ensure there is sending SRTP session.
		if (!this->srtpSendSession)
		{
			MS_WARN_TAG(rtcp, "ignoring RTCP compound packet due to non sending SRTP session");

			return;
		}

		if (!this->srtpSendSession->EncryptRtcp(&data, &intLen))
			return;

		auto len = static_cast<size_t>(intLen);

		this->iceServer->GetSelectedTuple()->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void WebRtcTransport::SendMessage(
	  RTC::DataConsumer* dataConsumer, uint32_t ppid, const uint8_t* msg, size_t len, onQueuedCallback* cb)
	{
		MS_TRACE();

		this->sctpAssociation->SendSctpMessage(dataConsumer, ppid, msg, len, cb);
	}

	void WebRtcTransport::SendSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// clang-format on
		if (!IsConnected())
		{
			MS_WARN_TAG(sctp, "DTLS not connected, cannot send SCTP data");

			return;
		}

		this->dtlsTransport->SendApplicationData(data, len);
	}

	void WebRtcTransport::RecvStreamClosed(uint32_t ssrc)
	{
		MS_TRACE();

		if (this->srtpRecvSession)
		{
			this->srtpRecvSession->RemoveStream(ssrc);
		}
	}

	void WebRtcTransport::SendStreamClosed(uint32_t ssrc)
	{
		MS_TRACE();

		if (this->srtpSendSession)
		{
			this->srtpSendSession->RemoveStream(ssrc);
		}
	}

	inline void WebRtcTransport::OnPacketReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Increase receive transmission.
		RTC::Transport::DataReceived(len);

		// Check if it's STUN.
		if (RTC::StunPacket::IsStun(data, len))
		{
			OnStunDataReceived(tuple, data, len);
		}
		// Check if it's RTCP.
		else if (RTC::RTCP::Packet::IsRtcp(data, len))
		{
			OnRtcpDataReceived(tuple, data, len);
		}
		// Check if it's RTP.
		else if (RTC::RtpPacket::IsRtp(data, len))
		{
			OnRtpDataReceived(tuple, data, len);
		}
		// Check if it's DTLS.
		else if (RTC::DtlsTransport::IsDtls(data, len))
		{
			OnDtlsDataReceived(tuple, data, len);
		}
		else
		{
			MS_WARN_DEV("ignoring received packet of unknown type");
		}
	}

	inline void WebRtcTransport::OnStunDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::StunPacket* packet = RTC::StunPacket::Parse(data, len);

		if (!packet)
		{
			MS_WARN_DEV("ignoring wrong STUN packet received");

			return;
		}

		// Pass it to the IceServer.
		this->iceServer->ProcessStunPacket(packet, tuple);

		delete packet;
	}

	inline void WebRtcTransport::OnDtlsDataReceived(
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
		  this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTING ||
		  this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTED)
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

	inline void WebRtcTransport::OnRtpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Ensure DTLS is connected.
		if (this->dtlsTransport->GetState() != RTC::DtlsTransport::DtlsState::CONNECTED)
		{
			MS_DEBUG_2TAGS(dtls, rtp, "ignoring RTP packet while DTLS not connected");

			return;
		}

		// Ensure there is receiving SRTP session.
		if (!this->srtpRecvSession)
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
		auto intLen = static_cast<int>(len);

		if (!this->srtpRecvSession->DecryptSrtp(const_cast<uint8_t*>(data), &intLen))
		{
			RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, static_cast<size_t>(intLen));

			if (!packet)
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

		RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, static_cast<size_t>(intLen));

		if (!packet)
		{
			MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

			return;
		}

		// Trick for clients performing aggressive ICE regardless we are ICE-Lite.
		this->iceServer->ForceSelectedTuple(tuple);

		// Pass the packet to the parent transport.
		RTC::Transport::ReceiveRtpPacket(packet);
	}

	inline void WebRtcTransport::OnRtcpDataReceived(
	  RTC::TransportTuple* tuple, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Ensure DTLS is connected.
		if (this->dtlsTransport->GetState() != RTC::DtlsTransport::DtlsState::CONNECTED)
		{
			MS_DEBUG_2TAGS(dtls, rtcp, "ignoring RTCP packet while DTLS not connected");

			return;
		}

		// Ensure there is receiving SRTP session.
		if (!this->srtpRecvSession)
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
		auto intLen = static_cast<int>(len);

		if (!this->srtpRecvSession->DecryptSrtcp(const_cast<uint8_t*>(data), &intLen))
			return;

		RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(data, static_cast<size_t>(intLen));

		if (!packet)
		{
			MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

			return;
		}

		// Pass the packet to the parent transport.
		RTC::Transport::ReceiveRtcpPacket(packet);
	}

	inline void WebRtcTransport::OnUdpSocketPacketReceived(
	  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(socket, remoteAddr);

		OnPacketReceived(&tuple, data, len);
	}

	inline void WebRtcTransport::OnRtcTcpConnectionClosed(
	  RTC::TcpServer* /*tcpServer*/, RTC::TcpConnection* connection)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		this->iceServer->RemoveTuple(&tuple);
	}

	inline void WebRtcTransport::OnTcpConnectionPacketReceived(
	  RTC::TcpConnection* connection, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		RTC::TransportTuple tuple(connection);

		OnPacketReceived(&tuple, data, len);
	}

	inline void WebRtcTransport::OnIceServerSendStunPacket(
	  const RTC::IceServer* /*iceServer*/, const RTC::StunPacket* packet, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Send the STUN response over the same transport tuple.
		tuple->Send(packet->GetData(), packet->GetSize());

		// Increase send transmission.
		RTC::Transport::DataSent(packet->GetSize());
	}

	inline void WebRtcTransport::OnIceServerLocalUsernameFragmentAdded(
	  const RTC::IceServer* /*iceServer*/, const std::string& usernameFragment)
	{
		MS_TRACE();

		if (this->webRtcTransportListener)
		{
			this->webRtcTransportListener->OnWebRtcTransportLocalIceUsernameFragmentAdded(
			  this, usernameFragment);
		}
	}

	inline void WebRtcTransport::OnIceServerLocalUsernameFragmentRemoved(
	  const RTC::IceServer* /*iceServer*/, const std::string& usernameFragment)
	{
		MS_TRACE();

		if (this->webRtcTransportListener)
		{
			this->webRtcTransportListener->OnWebRtcTransportLocalIceUsernameFragmentRemoved(
			  this, usernameFragment);
		}
	}

	inline void WebRtcTransport::OnIceServerTupleAdded(
	  const RTC::IceServer* /*iceServer*/, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		if (this->webRtcTransportListener)
		{
			this->webRtcTransportListener->OnWebRtcTransportTransportTupleAdded(this, tuple);
		}
	}

	inline void WebRtcTransport::OnIceServerTupleRemoved(
	  const RTC::IceServer* /*iceServer*/, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		if (this->webRtcTransportListener)
		{
			this->webRtcTransportListener->OnWebRtcTransportTransportTupleRemoved(this, tuple);
		}

		// If this is a TCP tuple, close its underlaying TCP connection.
		if (tuple->GetProtocol() == RTC::TransportTuple::Protocol::TCP && !tuple->IsClosed())
		{
			tuple->Close();
		}
	}

	inline void WebRtcTransport::OnIceServerSelectedTuple(
	  const RTC::IceServer* /*iceServer*/, RTC::TransportTuple* /*tuple*/)
	{
		MS_TRACE();

		/*
		 * RFC 5245 section 11.2 "Receiving Media":
		 *
		 * ICE implementations MUST be prepared to receive media on each component
		 * on any candidates provided for that component.
		 */

		MS_DEBUG_TAG(ice, "ICE selected tuple");

		// Notify the Node WebRtcTransport.
		json data = json::object();

		this->iceServer->GetSelectedTuple()->FillJson(data["iceSelectedTuple"]);

		this->shared->channelNotifier->Emit(this->id, "iceselectedtuplechange", data);
	}

	inline void WebRtcTransport::OnIceServerConnected(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		MS_DEBUG_TAG(ice, "ICE connected");

		// Notify the Node WebRtcTransport.
		json data = json::object();

		data["iceState"] = "connected";

		this->shared->channelNotifier->Emit(this->id, "icestatechange", data);

		// If ready, run the DTLS handler.
		MayRunDtlsTransport();

		// If DTLS was already connected, notify the parent class.
		if (this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTED)
		{
			RTC::Transport::Connected();
		}
	}

	inline void WebRtcTransport::OnIceServerCompleted(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		MS_DEBUG_TAG(ice, "ICE completed");

		// Notify the Node WebRtcTransport.
		json data = json::object();

		data["iceState"] = "completed";

		this->shared->channelNotifier->Emit(this->id, "icestatechange", data);

		// If ready, run the DTLS handler.
		MayRunDtlsTransport();

		// If DTLS was already connected, notify the parent class.
		if (this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTED)
		{
			RTC::Transport::Connected();
		}
	}

	inline void WebRtcTransport::OnIceServerDisconnected(const RTC::IceServer* /*iceServer*/)
	{
		MS_TRACE();

		MS_DEBUG_TAG(ice, "ICE disconnected");

		// Notify the Node WebRtcTransport.
		json data = json::object();

		data["iceState"] = "disconnected";

		this->shared->channelNotifier->Emit(this->id, "icestatechange", data);

		// If DTLS was already connected, notify the parent class.
		if (this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTED)
		{
			RTC::Transport::Disconnected();
		}
	}

	inline void WebRtcTransport::OnDtlsTransportConnecting(const RTC::DtlsTransport* /*dtlsTransport*/)
	{
		MS_TRACE();

		MS_DEBUG_TAG(dtls, "DTLS connecting");

		// Notify the Node WebRtcTransport.
		json data = json::object();

		data["dtlsState"] = "connecting";

		this->shared->channelNotifier->Emit(this->id, "dtlsstatechange", data);
	}

	inline void WebRtcTransport::OnDtlsTransportConnected(
	  const RTC::DtlsTransport* /*dtlsTransport*/,
	  RTC::SrtpSession::CryptoSuite srtpCryptoSuite,
	  uint8_t* srtpLocalKey,
	  size_t srtpLocalKeyLen,
	  uint8_t* srtpRemoteKey,
	  size_t srtpRemoteKeyLen,
	  std::string& remoteCert)
	{
		MS_TRACE();

		MS_DEBUG_TAG(dtls, "DTLS connected");

		// Close it if it was already set and update it.
		delete this->srtpSendSession;
		this->srtpSendSession = nullptr;

		delete this->srtpRecvSession;
		this->srtpRecvSession = nullptr;

		try
		{
			this->srtpSendSession = new RTC::SrtpSession(
			  RTC::SrtpSession::Type::OUTBOUND, srtpCryptoSuite, srtpLocalKey, srtpLocalKeyLen);
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR("error creating SRTP sending session: %s", error.what());
		}

		try
		{
			this->srtpRecvSession = new RTC::SrtpSession(
			  RTC::SrtpSession::Type::INBOUND, srtpCryptoSuite, srtpRemoteKey, srtpRemoteKeyLen);

			// Notify the Node WebRtcTransport.
			json data = json::object();

			data["dtlsState"]      = "connected";
			data["dtlsRemoteCert"] = remoteCert;

			this->shared->channelNotifier->Emit(this->id, "dtlsstatechange", data);

			// Tell the parent class.
			RTC::Transport::Connected();
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR("error creating SRTP receiving session: %s", error.what());

			delete this->srtpSendSession;
			this->srtpSendSession = nullptr;
		}
	}

	inline void WebRtcTransport::OnDtlsTransportFailed(const RTC::DtlsTransport* /*dtlsTransport*/)
	{
		MS_TRACE();

		MS_WARN_TAG(dtls, "DTLS failed");

		// Notify the Node WebRtcTransport.
		json data = json::object();

		data["dtlsState"] = "failed";

		this->shared->channelNotifier->Emit(this->id, "dtlsstatechange", data);
	}

	inline void WebRtcTransport::OnDtlsTransportClosed(const RTC::DtlsTransport* /*dtlsTransport*/)
	{
		MS_TRACE();

		MS_WARN_TAG(dtls, "DTLS remotely closed");

		// Notify the Node WebRtcTransport.
		json data = json::object();

		data["dtlsState"] = "closed";

		this->shared->channelNotifier->Emit(this->id, "dtlsstatechange", data);

		// Tell the parent class.
		RTC::Transport::Disconnected();
	}

	inline void WebRtcTransport::OnDtlsTransportSendData(
	  const RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!this->iceServer->GetSelectedTuple())
		{
			MS_WARN_TAG(dtls, "no selected tuple set, cannot send DTLS packet");

			return;
		}

		this->iceServer->GetSelectedTuple()->Send(data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	inline void WebRtcTransport::OnDtlsTransportApplicationDataReceived(
	  const RTC::DtlsTransport* /*dtlsTransport*/, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Pass it to the parent transport.
		RTC::Transport::ReceiveSctpData(data, len);
	}
} // namespace RTC
