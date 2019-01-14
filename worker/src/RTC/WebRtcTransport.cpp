#define MS_CLASS "RTC::WebRtcTransport"
// #define MS_LOG_DEV

#include "RTC/WebRtcTransport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <cmath>    // std::pow()
#include <iterator> // std::ostream_iterator
#include <sstream>  // std::ostringstream

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

	WebRtcTransport::WebRtcTransport(std::string& id, RTC::Transport::Listener* listener, Options& options)
	  : RTC::Transport::Transport(id, listener)
	{
		MS_TRACE();

		try
		{
			uint16_t initialIceLocalPreference = IceCandidateDefaultLocalPriority;

			for (auto &listenIp : options.listenIps)
			{
				uint16_t iceLocalPreference = initialIceLocalPreference;

				if (options.enableUdp)
				{
					if (options.preferUdp)
						iceLocalPreference += 1000;

					uint32_t icePriority = generateIceCandidatePriority(iceLocalPreference);

					// This may throw.
					auto* udpSocket = new RTC::UdpSocket(this, listenIp.ip);

					this->udpSockets.push_back(udpSocket);

					if (listenIp.announcedIp.empty())
					{
						RTC::IceCandidate iceCandidate(udpSocket, icePriority);

						this->iceLocalCandidates.push_back(iceCandidate);
					}
					else
					{
						RTC::IceCandidate iceCandidate(udpSocket, icePriority, listenIp.announcedIp);

						this->iceLocalCandidates.push_back(iceCandidate);
					}
				}

				if (options.enableTcp)
				{
					if (options.preferTcp)
						iceLocalPreference += 1000;

					uint32_t icePriority = generateIceCandidatePriority(iceLocalPreference);

					// This may throw.
					auto* tcpServer = new RTC::TcpServer(this, this, listenIp.ip);

					this->tcpServers.push_back(tcpServer);

					if (listenIp.announcedIp.empty())
					{
						RTC::IceCandidate iceCandidate(tcpServer, icePriority);

						this->iceLocalCandidates.push_back(iceCandidate);
					}
					else
					{
						RTC::IceCandidate iceCandidate(tcpServer, icePriority, listenIp.announcedIp);

						this->iceLocalCandidates.push_back(iceCandidate);
					}
				}

				// Decrement initial ICE local preference for next IP.
				initialIceLocalPreference -= 100;
			}

			// Create a ICE server.
			this->iceServer = new RTC::IceServer(
			  this, Utils::Crypto::GetRandomString(16), Utils::Crypto::GetRandomString(32));

			// Create a DTLS transport.
			this->dtlsTransport = new RTC::DtlsTransport(this);
		}
		catch (const MediaSoupError& error)
		{
			MS_ERROR("constructor failed: %s", error.what());

			// Must delete everything since the destructor won't be called.

			if (this->dtlsTransport != nullptr)
				delete this->dtlsTransport;

			if (this->iceServer != nullptr)
				delete this->iceServer;

			for (auto* socket : this->udpSockets)
			{
				delete socket;
			}
			this->udpSockets.clear();

			for (auto* server : this->tcpServers)
			{
				delete server;
			}
			this->tcpServers.clear();

			throw error;
		}
	}

	WebRtcTransport::~WebRtcTransport()
	{
		MS_TRACE();

		// Must delete the DTLS transport first since it will generate a DTLS alert
		// to be sent.
		if (this->dtlsTransport != nullptr)
			delete this->dtlsTransport;

		if (this->iceServer != nullptr)
			delete this->iceServer;

		for (auto* socket : this->udpSockets)
		{
			delete socket;
		}
		this->udpSockets.clear();

		for (auto* server : this->tcpServers)
		{
			delete server;
		}
		this->tcpServers.clear();

		if (this->srtpRecvSession != nullptr)
			delete this->srtpRecvSession;

		if (this->srtpSendSession != nullptr)
			delete this->srtpSendSession;
	}

	WebRtcTransport::Close()
	{
		MS_TRACE();

		// Must delete the DTLS transport first since it will generate a DTLS alert
		// to be sent.
		if (this->dtlsTransport != nullptr)
		{
			delete this->dtlsTransport;
			this->dtlsTransport == nullptr;
		}

		if (this->iceServer != nullptr)
		{
			delete this->iceServer;
			this->iceServer == nullptr;
		}

		for (auto* socket : this->udpSockets)
		{
			delete socket;
		}
		this->udpSockets.clear();

		for (auto* server : this->tcpServers)
		{
			delete server;
		}
		this->tcpServers.clear();

		if (this->srtpRecvSession != nullptr)
		{
			delete this->srtpRecvSession;
			this->srtpRecvSession = null;
		}

		if (this->srtpSendSession != nullptr)
		{
			delete this->srtpSendSession;
			this->srtpSendSession = nullptr;
		}

		// Also call the parent method.
		RTC::Transport::Close();
	}

	void WebRtcTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add iceRole (we are always "controlled").
		jsonObject["iceRole"] = "controlled";

		// Add iceLocalParameters.
		jsonObject["iceLocalParameters"] = json::object();
		auto jsonIceLocalParametersIt    = jsonObject.find("iceLocalParameters");

		(*jsonIceLocalParametersIt)["usernameFragment"] = this->iceServer->GetUsernameFragment();
		(*jsonIceLocalParametersIt)["password"]         = this->iceServer->GetPassword();
		(*jsonIceLocalParametersIt)["iceLite"]          = true;

		// Add iceLocalCandidates.
		jsonObject["iceLocalCandidates"] = json::array();
		auto jsonIceLocalCandidatesIt    = jsonObject.find("iceLocalCandidates");

		for (auto i = 0; i < this->iceLocalCandidates.size(); ++i)
		{
			jsonIceLocalCandidatesIt->emplace_back(json::value_t::object);

			auto& jsonEntry    = (*jsonIceLocalCandidatesIt)[i];
			auto& iceCandidate = this->iceLocalCandidates[i];

			iceCandidate.FillJson(jsonEntry);
		}

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

		// Add iceSelectedTuple.
		if (this->selectedTuple != nullptr)
			this->selectedTuple->FillJson(jsonObject["iceSelectedTuple"]);

		// Add dtlsLocalParameters.
		jsonObject["dtlsLocalParameters"] = json::object();
		auto jsonDtlsLocalParametersIt    = jsonObject.find("dtlsLocalParameters");

		// Add dtlsLocalParameters.fingerprints.
		(*jsonDtlsLocalParametersIt)["fingerprints"] = json::array();
		auto jsonDtlsLocalParametersFingerprintsIt   = jsonDtlsLocalParametersIt->find("fingerprints");
		auto& fingerprints                           = this->dtlsTransport->GetLocalFingerprints();

		for (auto i = 0; i < fingerprints.size(); ++i)
		{
			jsonDtlsLocalParametersFingerprintsIt->emplace_back(json::value_t::object);

			auto& jsonEntry   = (*jsonDtlsLocalParametersFingerprintsIt)[i];
			auto& fingerprint = fingerprints[i];

			jsonEntry["algorithm"] =
			  RTC::DtlsTransport::GetFingerprintAlgorithmString(fingerprint.algorithm);
			jsonEntry["value"] = fingerprint.value;
		}

		// Add dtlsLocalParameters.role.
		switch (this->dtlsLocalRole)
		{
			case RTC::DtlsTransport::Role::NONE:
				(*jsonDtlsLocalParametersIt)["role"] = "none";
				break;
			case RTC::DtlsTransport::Role::AUTO:
				(*jsonDtlsLocalParametersIt)["role"] = "auto";
				break;
			case RTC::DtlsTransport::Role::CLIENT:
				(*jsonDtlsLocalParametersIt)["role"] = "client";
				break;
			case RTC::DtlsTransport::Role::SERVER:
				(*jsonDtlsLocalParametersIt)["role"] = "server";
				break;
		}

		// Add dtlsState.
		switch (this->dtlsTransport->GetState())
		{
			case DtlsTransport::DtlsState::NEW:
				jsonObject["dtlsState"] = "new";
				break;
			case DtlsTransport::DtlsState::CONNECTING:
				jsonObject["dtlsState"] = "connecting";
				break;
			case DtlsTransport::DtlsState::CONNECTED:
				jsonObject["dtlsState"] = "connected";
				break;
			case DtlsTransport::DtlsState::FAILED:
				jsonObject["dtlsState"] = "failed";
				break;
			case DtlsTransport::DtlsState::CLOSED:
				jsonObject["dtlsState"] = "closed";
				break;
		}

		// Add headerExtensionIds.
		jsonObject["headerExtensions"] = json::object();
		auto jsonHeaderExtensionsIt    = jsonObject.find("headerExtensions");

		if (this->rtpHeaderExtensionIds.absSendTime != 0u)
			(*jsonHeaderExtensionsIt)["absSendTime"] = this->rtpHeaderExtensionIds.absSendTime;

		if (this->rtpHeaderExtensionIds.mid != 0u)
			(*jsonHeaderExtensionsIt)["mid"] = this->rtpHeaderExtensionIds.mid;

		if (this->rtpHeaderExtensionIds.rid != 0u)
			(*jsonHeaderExtensionsIt)["rid"] = this->rtpHeaderExtensionIds.rid;

		// Add rtpListener.
		this->rtpListener.FillJson(jsonObject["rtpListener"]);
	}

	void WebRtcTransport::FillJsonStats(json& jsonObject) const
	{
		MS_TRACE();

		// Add type.
		// Add type.
		jsonObject["type"] = "transport";

		// Add transportId.
		jsonObject["transportId"] = this->id;

		// Add timestamp.
		jsonObject["timestamp"] = DepLibUV::GetTime();

		// Add iceConnectionState.
		switch (this->iceServer->GetState())
		{
			case RTC::IceServer::IceState::NEW:
				jsonObject["iceConnectionState"] = "new";
				break;
			case RTC::IceServer::IceState::CONNECTED:
				jsonObject["iceConnectionState"] = "connected";
				break;
			case RTC::IceServer::IceState::COMPLETED:
				jsonObject["iceConnectionState"] = "completed";
				break;
			case RTC::IceServer::IceState::DISCONNECTED:
				jsonObject["iceConnectionState"] = "disconnected";
				break;
		}

		// Add dtlsState.
		switch (this->dtlsTransport->GetState())
		{
			case DtlsTransport::DtlsState::NEW:
				jsonObject["dtlsState"] = "new";
				break;
			case DtlsTransport::DtlsState::CONNECTING:
				jsonObject["dtlsState"] = "connecting";
				break;
			case DtlsTransport::DtlsState::CONNECTED:
				jsonObject["dtlsState"] = "connected";
				break;
			case DtlsTransport::DtlsState::FAILED:
				jsonObject["dtlsState"] = "failed";
				break;
			case DtlsTransport::DtlsState::CLOSED:
				jsonObject["dtlsState"] = "closed";
				break;
		}

		if (this->selectedTuple != nullptr)
		{
			// Add bytesReceived.
			jsonObject["bytesReceived"] = this->selectedTuple->GetRecvBytes();
			// Add bytesSent.
			jsonObject["bytesSent"] = this->selectedTuple->GetSentBytes();
			// Add iceSelectedTuple.
			this->selectedTuple->FillJson(jsonObject["iceSelectedTuple"]);
		}

		// Add availableIncomingBitrate.
		if (this->availableIncomingBitrate != 0u)
			jsonObject["availableIncomingBitrate"] = this->availableIncomingBitrate;

		// Add availableOutgoingBitrate.
		if (this->availableOutgoingBitrate != 0u)
			jsonObject["availableOutgoingBitrate"] = this->availableOutgoingBitrate;

		// Add maxIncomingBitrate.
		if (this->maxIncomingBitrate != 0u)
			jsonObject["maxIncomingBitrate"] = this->maxIncomingBitrate;
	}

	void Router::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_CONNECT:
			{
				// Ensure this method is not called twice.
				if (this->dtlsLocalRole != RTC::DtlsTransport::Role::AUTO)
				{
					request->Reject("remote parameters already set");

					return;
				}

				RTC::DtlsTransport::Fingerprint dtlsRemoteFingerprint;
				RTC::DtlsTransport::Role dtlsRemoteRole;

				try
				{
					auto jsonDtlsParametersIt = request->data.find("dtlsParameters");

					if (jsonDtlsParametersIt == request->data.end() || !jsonDtlsParametersIt->is_object())
						MS_THROW_ERROR("missing dtlsParameters");

					auto jsonFingerprintsIt = jsonDtlsParametersIt->find("fingerprints");

					if (jsonFingerprintsIt == jsonDtlsParametersIt->end() || !jsonFingerprintsIt->is_array())
						MS_THROW_ERROR("missing dtlsParameters.fingerprints");

					// NOTE: Just take the first fingerprint.
					for (auto& jsonFingerprint : *jsonFingerprintsIt)
					{
						if (!jsonFingerprint.is_object())
							MS_THROW_ERROR("wrong entry in dtlsParameters.fingerprints (not an object)");

						auto jsonAlgorithmIt = jsonFingerprint.find("algorithm");

						if (jsonAlgorithmIt == jsonFingerprint.end())
							MS_THROW_ERROR("missing fingerprint.algorithm");
						else if (!jsonAlgorithmIt->is_string())
							MS_THROW_ERROR("wrong fingerprint.algorithm (not a string)");

						dtlsRemoteFingerprint.algorithm = RTC::DtlsTransport::GetFingerprintAlgorithm(
						  jsonAlgorithmIt->get<std:string>());

						if (dtlsRemoteFingerprint.algorithm == RTC::DtlsTransport::FingerprintAlgorithm::NONE)
							MS_THROW_ERROR("invalid fingerprint.algorithm value");

						auto jsonValueIt = jsonFingerprint.find("value");

						if (jsonValueIt == jsonFingerprint.end())
							MS_THROW_ERROR("missing fingerprint.value");
						else if (!jsonValueIt->is_string())
							MS_THROW_ERROR("wrong fingerprint.value (not a string)");

						dtlsRemoteFingerprint.value = jsonValueIt->get<std::string>();

						// Just use the first fingerprint.
						break;
					}

					auto jsonRoleIt = jsonDtlsParametersIt->find("role");

					if (jsonRoleIt != jsonDtlsParametersIt->end())
					{
						if (!jsonRoleIt->is_string())
							MS_THROW_ERROR("wrong dtlsParameters.role (not a string)");

						dtlsRemoteRole = RTC::DtlsTransport::StringToRole(jsonRoleIt->get<std::string>());

						if (dtlsRemoteRole == RTC::DtlsTransport::Role::NONE)
							MS_THROW_ERROR("invalid dtlsParameters.role value");
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
							this->dtlsLocalRole = RTC::DtlsTransport::Role::SERVER;

							break;
						}
						case RTC::DtlsTransport::Role::SERVER:
						{
							this->dtlsLocalRole = RTC::DtlsTransport::Role::CLIENT;

							break;
						}
						// If the peer has role "auto" we become "client" since we are ICE controlled.
						case RTC::DtlsTransport::Role::AUTO:
						{
							this->dtlsLocalRole = RTC::DtlsTransport::Role::CLIENT;

							break;
						}
						case RTC::DtlsTransport::Role::NONE:
						{
							MS_THROW_ERROR("invalid remote DTLS role");
						}
					}
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// Pass the remote fingerprint to the DTLS transport.
				if (this->dtlsTransport->SetRemoteFingerprint(dtlsRemoteFingerprint))
				{
					// If everything is fine, we may run the DTLS transport if ready.
					MayRunDtlsTransport();
				}

				// Start the RTCP timer.
				this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));

				// Set remote bitrate estimator.
				this->rembRemoteBitrateEstimator.reset(new RTC::REMB::RemoteBitrateEstimatorAbsSendTime(this));

				// Tell the caller about the selected local DTLS role.
				json data = json::object();

				switch (this->dtlsLocalRole)
				{
					case RTC::DtlsTransport::Role::CLIENT:
						data["dtlsLocalRole"] = "client";
						break;
					case RTC::DtlsTransport::Role::SERVER:
						data["dtlsLocalRole"] = "server";
						break;
					default:
						MS_ABORT("invalid local DTLS role");
				}

				request->Accept(data);

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Transport::HandleRequest(request);
			}
		}
	}

	// TODO: Not here but in HandleRequest().
	// TODO: No, the new ICE username and password must be decided by this method.
	// TODO: This must return the new local RTCIceParameters, including iceLite: true.
	void WebRtcTransport::RestartIce(std::string& usernameFragment, std::string& password)
	{
		MS_TRACE();

		this->iceServer->SetUsernameFragment(usernameFragment);
		this->iceServer->SetPassword(password);

		MS_DEBUG_DEV("Transport ICE ufrag&pwd changed [id:%" PRIu32 "]", this->id);
	}

	inline bool WebRtcTransport::IsConnected() const
	{
		return (
		  this->selectedTuple != nullptr &&
		  this->dtlsTransport->GetState() == RTC::DtlsTransport::DtlsState::CONNECTED);
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

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// Ensure there is sending SRTP session.
		if (this->srtpSendSession == nullptr)
		{
			MS_WARN_DEV("ignoring RTCP packet due to non sending SRTP session");

			return;
		}

		if (!this->srtpSendSession->EncryptRtcp(&data, &len))
			return;

		this->selectedTuple->Send(data, len);
	}

	void WebRtcTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		if (!IsConnected())
			return;

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// Ensure there is sending SRTP session.
		if (this->srtpSendSession == nullptr)
		{
			MS_DEBUG_DEV("ignoring RTCP compound packet due to non sending SRTP session");

			return;
		}

		if (!this->srtpSendSession->EncryptRtcp(&data, &len))
			return;

		this->selectedTuple->Send(data, len);
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
			//
			// NOTE: This is the theory, however let's be mor eflexible as told here:
			//   https://bugs.chromium.org/p/webrtc/issues/detail?id=3661
			case RTC::DtlsTransport::Role::CLIENT:
				if (
				  this->iceServer->GetState() == RTC::IceServer::IceState::CONNECTED ||
				  this->iceServer->GetState() == RTC::IceServer::IceState::COMPLETED)
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
		packet->SetAbsSendTimeExtensionId(this->rtpHeaderExtensionIds.absSendTime);
		packet->SetMidExtensionId(this->rtpHeaderExtensionIds.mid);
		packet->SetRidExtensionId(this->rtpHeaderExtensionIds.rid);

		// Feed the remote bitrate estimator (REMB).
		uint32_t absSendTime;

		if (packet->ReadAbsSendTime(&absSendTime))
		{
			this->rembRemoteBitrateEstimator->IncomingPacket(
			  DepLibUV::GetTime(), packet->GetPayloadLength(), *packet, absSendTime);
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
			ReceiveRtcpPacket(packet);

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
		eventData["tuple"] = tuple->ToJson();
		Channel::Notifier::Emit(this->id, "iceselectedtuplechange", eventData);
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
		Channel::Notifier::Emit(this->id, "icestatechange", eventData);

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
		Channel::Notifier::Emit(this->id, "icestatechange", eventData);

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
		Channel::Notifier::Emit(this->id, "icestatechange", eventData);
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
		Channel::Notifier::Emit(this->id, "dtlsstatechange", eventData);
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
			delete this->srtpSendSession;
			this->srtpSendSession = nullptr;
		}
		if (this->srtpRecvSession != nullptr)
		{
			delete this->srtpRecvSession;
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

			delete this->srtpSendSession;
			this->srtpSendSession = nullptr;
		}

		// Notify.
		eventData[JsonStringDtlsState]      = JsonStringConnected;
		eventData[JsonStringDtlsRemoteCert] = remoteCert;
		Channel::Notifier::Emit(this->id, "dtlsstatechange", eventData);

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
		Channel::Notifier::Emit(this->id, "dtlsstatechange", eventData);

		// This is a fatal error so close the transport.
		Close();
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
		Channel::Notifier::Emit(this->id, "dtlsstatechange", eventData);

		// This is a fatal error so close the transport.
		Close();
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

	void WebRtcTransport::OnRemoteBitrateEstimatorValue(const RemoteBitrateEstimator* /*remoteBitrateEstimator*/, const std::vector<uint32_t>& ssrcs, uint32_t availableBitrate)
	{
		MS_TRACE();

		this->availableIncomingBitrate = availableBitrate;

		uint32_t effectiveMaxBitrate{ 0u };

		// Limit announced available bitrate if requested via API.
		if (this->maxIncomingBitrate != 0u)
			effectiveMaxBitrate = std::min(availableBitrate, this->maxIncomingBitrate);
		else
			effectiveMaxBitrate = availableBitrate;

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
			  "sending RTCP REMB packet [available:%" PRIu32 "bps, effective:%" PRIu32 "bps, ssrcs:%s]",
			  availableBitrate,
			  effectiveMaxBitrate,
			  ssrcsStream.str().c_str());
		}

		RTC::RTCP::FeedbackPsRembPacket packet(0, 0);

		packet.SetBitrate(effectiveMaxBitrate);
		packet.SetSsrcs(ssrcs);
		packet.Serialize(RTC::RTCP::Buffer);
		SendRtcpPacket(&packet);
	}
} // namespace RTC
