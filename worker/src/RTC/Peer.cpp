#define MS_CLASS "RTC::Peer"
// #define MS_LOG_DEV

#include "RTC/Peer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RtpDictionaries.hpp"

namespace RTC
{
	/* Instance methods. */

	Peer::Peer(Listener* listener, Channel::Notifier* notifier, uint32_t peerId, std::string& peerName)
	    : peerId(peerId), peerName(peerName), listener(listener), notifier(notifier)
	{
		MS_TRACE();

		this->timer = new Timer(this);

		// Start the RTCP timer.
		this->timer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));
	}

	Peer::~Peer()
	{
		MS_TRACE();

		// Destroy the RTCP timer.
		this->timer->Destroy();
	}

	void Peer::Destroy()
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };

		Json::Value eventData(Json::objectValue);

		// Close all the RtpReceivers.
		for (auto it = this->rtpReceivers.begin(); it != this->rtpReceivers.end();)
		{
			auto* rtpReceiver = it->second;

			it = this->rtpReceivers.erase(it);
			rtpReceiver->Destroy();
		}

		// Close all the RtpSenders.
		for (auto it = this->rtpSenders.begin(); it != this->rtpSenders.end();)
		{
			auto* rtpSender = it->second;

			it = this->rtpSenders.erase(it);
			rtpSender->Destroy();
		}

		// Close all the Transports.
		// NOTE: It is critical to close Transports after RtpReceivers/RtpSenders
		// because RtcReceiver.Destroy() fires an event in the Transport.
		for (auto it = this->transports.begin(); it != this->transports.end();)
		{
			auto* transport = it->second;

			it = this->transports.erase(it);
			transport->Destroy();
		}

		// Notify.
		eventData[JsonStringClass] = "Peer";
		this->notifier->Emit(this->peerId, "close", eventData);

		// Notify the listener.
		this->listener->OnPeerClosed(this);

		delete this;
	}

	Json::Value Peer::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringPeerId{ "peerId" };
		static const Json::StaticString JsonStringPeerName{ "peerName" };
		static const Json::StaticString JsonStringCapabilities{ "capabilities" };
		static const Json::StaticString JsonStringTransports{ "transports" };
		static const Json::StaticString JsonStringRtpReceivers{ "rtpReceivers" };
		static const Json::StaticString JsonStringRtpSenders{ "rtpSenders" };

		Json::Value json(Json::objectValue);
		Json::Value jsonTransports(Json::arrayValue);
		Json::Value jsonRtpReceivers(Json::arrayValue);
		Json::Value jsonRtpSenders(Json::arrayValue);

		// Add `peerId`.
		json[JsonStringPeerId] = Json::UInt{ this->peerId };

		// Add `peerName`.
		json[JsonStringPeerName] = this->peerName;

		// Add `capabilities`.
		if (this->hasCapabilities)
			json[JsonStringCapabilities] = this->capabilities.ToJson();

		// Add `transports`.
		for (auto& kv : this->transports)
		{
			auto* transport = kv.second;

			jsonTransports.append(transport->ToJson());
		}
		json[JsonStringTransports] = jsonTransports;

		// Add `rtpReceivers`.
		for (auto& kv : this->rtpReceivers)
		{
			auto* rtpReceiver = kv.second;

			jsonRtpReceivers.append(rtpReceiver->ToJson());
		}
		json[JsonStringRtpReceivers] = jsonRtpReceivers;

		// Add `rtpSenders`.
		for (auto& kv : this->rtpSenders)
		{
			auto* rtpSender = kv.second;

			jsonRtpSenders.append(rtpSender->ToJson());
		}
		json[JsonStringRtpSenders] = jsonRtpSenders;

		return json;
	}

	void Peer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::PEER_CLOSE:
			{
#ifdef MS_LOG_DEV
				uint32_t peerId = this->peerId;
#endif

				Destroy();

				MS_DEBUG_DEV("Peer closed [peerId:%" PRIu32 "]", peerId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PEER_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::PEER_SET_CAPABILITIES:
			{
				// Capabilities must not be set.
				if (this->hasCapabilities)
				{
					request->Reject("peer capabilities already set");

					return;
				}

				try
				{
					this->capabilities = RTC::RtpCapabilities(request->data, RTC::Scope::PEER_CAPABILITY);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				this->hasCapabilities = true;

				// Notify the listener (Room) who will remove capabilities to make them
				// a subset of the room capabilities.
				this->listener->OnPeerCapabilities(this, std::addressof(this->capabilities));

				Json::Value data = this->capabilities.ToJson();

				// NOTE: We accept the request *after* calling onPeerCapabilities(). This
				// guarantees that the Peer will receive a "newrtpsender" event for all its
				// associated RtpSenders *before* the setCapabilities() Promise resolves.
				// In other words, at the time setCapabilities() resolves, the Peer already
				// has set all its current RtpSenders.
				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::PEER_CREATE_TRANSPORT:
			{
				RTC::Transport* transport;
				uint32_t transportId;

				try
				{
					transport = GetTransportFromRequest(request, &transportId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (transport != nullptr)
				{
					request->Reject("Transport already exists");

					return;
				}

				try
				{
					transport = new RTC::Transport(this, this->notifier, transportId, request->data);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				this->transports[transportId] = transport;

				MS_DEBUG_DEV("Transport created [transportId:%" PRIu32 "]", transportId);

				auto data = transport->ToJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::PEER_CREATE_RTP_RECEIVER:
			{
				static const Json::StaticString JsonStringKind{ "kind" };

				RTC::RtpReceiver* rtpReceiver;
				RTC::Transport* transport{ nullptr };
				uint32_t rtpReceiverId;

				// Capabilities must be set.
				if (!this->hasCapabilities)
				{
					request->Reject("peer capabilities are not yet set");

					return;
				}

				try
				{
					rtpReceiver = GetRtpReceiverFromRequest(request, &rtpReceiverId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (rtpReceiver != nullptr)
				{
					request->Reject("RtpReceiver already exists");

					return;
				}

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (transport == nullptr)
				{
					request->Reject("Transport does not exist");

					return;
				}

				// `kind` is mandatory.

				if (!request->data[JsonStringKind].isString())
					MS_THROW_ERROR("missing kind");

				std::string kind = request->data[JsonStringKind].asString();

				// Create a RtpReceiver instance.
				try
				{
					rtpReceiver =
					    new RTC::RtpReceiver(this, this->notifier, rtpReceiverId, RTC::Media::GetKind(kind));
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				this->rtpReceivers[rtpReceiverId] = rtpReceiver;

				MS_DEBUG_DEV("RtpReceiver created [rtpReceiverId:%" PRIu32 "]", rtpReceiverId);

				// Set the Transport.
				rtpReceiver->SetTransport(transport);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_CLOSE:
			case Channel::Request::MethodId::TRANSPORT_DUMP:
			case Channel::Request::MethodId::TRANSPORT_SET_REMOTE_DTLS_PARAMETERS:
			case Channel::Request::MethodId::TRANSPORT_SET_MAX_BITRATE:
			case Channel::Request::MethodId::TRANSPORT_CHANGE_UFRAG_PWD:
			{
				RTC::Transport* transport;

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (transport == nullptr)
				{
					request->Reject("Transport does not exist");

					return;
				}

				transport->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::RTP_RECEIVER_CLOSE:
			case Channel::Request::MethodId::RTP_RECEIVER_DUMP:
			case Channel::Request::MethodId::RTP_RECEIVER_RECEIVE:
			case Channel::Request::MethodId::RTP_RECEIVER_SET_RTP_RAW_EVENT:
			case Channel::Request::MethodId::RTP_RECEIVER_SET_RTP_OBJECT_EVENT:
			{
				RTC::RtpReceiver* rtpReceiver;

				try
				{
					rtpReceiver = GetRtpReceiverFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (rtpReceiver == nullptr)
				{
					request->Reject("RtpReceiver does not exist");

					return;
				}

				rtpReceiver->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::RTP_RECEIVER_SET_TRANSPORT:
			{
				RTC::RtpReceiver* rtpReceiver;

				try
				{
					rtpReceiver = GetRtpReceiverFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (rtpReceiver == nullptr)
				{
					request->Reject("RtpReceiver does not exist");

					return;
				}

				RTC::Transport* transport;

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (transport == nullptr)
				{
					request->Reject("Transport does not exist");

					return;
				}

				try
				{
					// NOTE: This may throw.
					transport->AddRtpReceiver(rtpReceiver);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// Enable REMB in the new transport if it was enabled in the previous one.
				auto previousTransport = rtpReceiver->GetTransport();

				if ((previousTransport != nullptr) && previousTransport->HasRemb())
					transport->EnableRemb();

				rtpReceiver->SetTransport(transport);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_SENDER_DUMP:
			{
				RTC::RtpSender* rtpSender;

				try
				{
					rtpSender = GetRtpSenderFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (rtpSender == nullptr)
				{
					request->Reject("RtpSender does not exist");

					return;
				}

				rtpSender->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::RTP_SENDER_SET_TRANSPORT:
			{
				RTC::RtpSender* rtpSender;

				try
				{
					rtpSender = GetRtpSenderFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (rtpSender == nullptr)
				{
					request->Reject("RtpSender does not exist");

					return;
				}

				RTC::Transport* transport;

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (transport == nullptr)
				{
					request->Reject("Transport does not exist");

					return;
				}

				rtpSender->SetTransport(transport);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_SENDER_DISABLE:
			{
				RTC::RtpSender* rtpSender;

				try
				{
					rtpSender = GetRtpSenderFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (rtpSender == nullptr)
				{
					request->Reject("RtpSender does not exist");

					return;
				}

				rtpSender->HandleRequest(request);

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void Peer::AddRtpSender(
	    RTC::RtpSender* rtpSender, RTC::RtpParameters* rtpParameters, uint32_t associatedRtpReceiverId)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };
		static const Json::StaticString JsonStringRtpSenderId{ "rtpSenderId" };
		static const Json::StaticString JsonStringKind{ "kind" };
		static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
		static const Json::StaticString JsonStringActive{ "active" };
		static const Json::StaticString JsonStringAssociatedRtpReceiverId{ "associatedRtpReceiverId" };

		MS_ASSERT(
		    this->rtpSenders.find(rtpSender->rtpSenderId) == this->rtpSenders.end(),
		    "given RtpSender already exists in this Peer");

		// Provide the RtpSender with peer's capabilities.
		rtpSender->SetPeerCapabilities(std::addressof(this->capabilities));

		// Provide the RtpSender with the received RTP parameters.
		rtpSender->Send(rtpParameters);

		// Store it.
		this->rtpSenders[rtpSender->rtpSenderId] = rtpSender;

		// Notify.
		Json::Value eventData = rtpSender->ToJson();

		eventData[JsonStringClass]                   = "Peer";
		eventData[JsonStringRtpSenderId]             = Json::UInt{ rtpSender->rtpSenderId };
		eventData[JsonStringKind]                    = RTC::Media::GetJsonString(rtpSender->kind);
		eventData[JsonStringRtpParameters]           = rtpSender->GetParameters()->ToJson();
		eventData[JsonStringActive]                  = rtpSender->GetActive();
		eventData[JsonStringAssociatedRtpReceiverId] = Json::UInt{ associatedRtpReceiverId };

		this->notifier->Emit(this->peerId, "newrtpsender", eventData);
	}

	RTC::RtpSender* Peer::GetRtpSender(uint32_t ssrc) const
	{
		MS_TRACE();

		auto it = this->rtpSenders.begin();
		for (; it != this->rtpSenders.end(); ++it)
		{
			auto rtpSender     = it->second;
			auto rtpParameters = rtpSender->GetParameters();

			if (rtpParameters == nullptr)
				continue;

			auto it2 = rtpParameters->encodings.begin();
			for (; it2 != rtpParameters->encodings.end(); ++it2)
			{
				auto& encoding = *it2;

				if (encoding.ssrc == ssrc)
					return rtpSender;
				if (encoding.hasFec && encoding.fec.ssrc == ssrc)
					return rtpSender;
				if (encoding.hasRtx && encoding.rtx.ssrc == ssrc)
					return rtpSender;
			}
		}

		return nullptr;
	}

	void Peer::SendRtcp(uint64_t now)
	{
		MS_TRACE();

		// For every transport:
		// - Create a CompoundPacket.
		// - Request every Sender and Receiver of such transport their RTCP data.
		// - Send the CompoundPacket.

		for (auto& it : this->transports)
		{
			std::unique_ptr<RTC::RTCP::CompoundPacket> packet(new RTC::RTCP::CompoundPacket());
			auto* transport = it.second;

			for (auto& it : this->rtpSenders)
			{
				auto* rtpSender = it.second;

				if (rtpSender->GetTransport() != transport)
					continue;

				rtpSender->GetRtcp(packet.get(), now);

				// Send one RTCP compound packet per sender report.
				if (packet->GetSenderReportCount() != 0u)
				{
					// Ensure that the RTCP packet fits into the RTCP buffer.
					if (packet->GetSize() > RTC::RTCP::BufferSize)
					{
						MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

						return;
					}

					packet->Serialize(RTC::RTCP::Buffer);
					transport->SendRtcpCompoundPacket(packet.get());
					// Reset the Compound packet.
					packet.reset(new RTC::RTCP::CompoundPacket());
				}
			}

			for (auto& it : this->rtpReceivers)
			{
				auto* rtpReceiver = it.second;

				if (rtpReceiver->GetTransport() != transport)
					continue;

				rtpReceiver->GetRtcp(packet.get(), now);
			}

			// Send one RTCP compound with all receiver reports.
			if (packet->GetReceiverReportCount() != 0u)
			{
				// Ensure that the RTCP packet fits into the RTCP buffer.
				if (packet->GetSize() > RTC::RTCP::BufferSize)
				{
					MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());

					return;
				}

				packet->Serialize(RTC::RTCP::Buffer);
				transport->SendRtcpCompoundPacket(packet.get());
			}
		}
	}

	RTC::Transport* Peer::GetTransportFromRequest(Channel::Request* request, uint32_t* transportId) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTransportId{ "transportId" };

		auto jsonTransportId = request->internal[JsonStringTransportId];

		if (!jsonTransportId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.transportId");

		if (transportId != nullptr)
			*transportId = jsonTransportId.asUInt();

		auto it = this->transports.find(jsonTransportId.asUInt());
		if (it != this->transports.end())
		{
			auto* transport = it->second;

			return transport;
		}

		return nullptr;
	}

	RTC::RtpReceiver* Peer::GetRtpReceiverFromRequest(Channel::Request* request, uint32_t* rtpReceiverId) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringRtpReceiverId{ "rtpReceiverId" };

		auto jsonRtpReceiverId = request->internal[JsonStringRtpReceiverId];

		if (!jsonRtpReceiverId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.rtpReceiverId");

		if (rtpReceiverId != nullptr)
			*rtpReceiverId = jsonRtpReceiverId.asUInt();

		auto it = this->rtpReceivers.find(jsonRtpReceiverId.asUInt());
		if (it != this->rtpReceivers.end())
		{
			auto* rtpReceiver = it->second;

			return rtpReceiver;
		}

		return nullptr;
	}

	RTC::RtpSender* Peer::GetRtpSenderFromRequest(Channel::Request* request, uint32_t* rtpSenderId) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringRtpSenderId{ "rtpSenderId" };

		auto jsonRtpSenderId = request->internal[JsonStringRtpSenderId];

		if (!jsonRtpSenderId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.rtpSenderId");

		if (rtpSenderId != nullptr)
			*rtpSenderId = jsonRtpSenderId.asUInt();

		auto it = this->rtpSenders.find(jsonRtpSenderId.asUInt());
		if (it != this->rtpSenders.end())
		{
			auto* rtpSender = it->second;

			return rtpSender;
		}

		return nullptr;
	}

	void Peer::OnTransportConnected(RTC::Transport* transport)
	{
		MS_TRACE();

		// If the transport is used by any RtpSender (video/depth) notify the
		// listener.
		for (auto& kv : this->rtpSenders)
		{
			auto* rtpSender = kv.second;

			if (rtpSender->kind != RTC::Media::Kind::VIDEO && rtpSender->kind != RTC::Media::Kind::DEPTH)
			{
				continue;
			}

			if (rtpSender->GetTransport() != transport)
				continue;

			this->listener->OnFullFrameRequired(this, rtpSender);
		}
	}

	void Peer::OnTransportClosed(RTC::Transport* transport)
	{
		MS_TRACE();

		// Must remove the closed Transport from all the RtpReceivers holding it.
		for (auto& kv : this->rtpReceivers)
		{
			auto* rtpReceiver = kv.second;

			rtpReceiver->RemoveTransport(transport);
		}

		// Must also unset this Transport from all the RtpSenders using it.
		for (auto& kv : this->rtpSenders)
		{
			auto* rtpSender = kv.second;

			rtpSender->RemoveTransport(transport);
		}

		this->transports.erase(transport->transportId);
	}

	void Peer::OnTransportFullFrameRequired(RTC::Transport* transport)
	{
		MS_TRACE();

		// If the transport is used by any RtpReceiver (video/depth) notify the
		// listener.
		for (auto& kv : this->rtpReceivers)
		{
			auto* rtpReceiver = kv.second;

			if (rtpReceiver->kind != RTC::Media::Kind::VIDEO && rtpReceiver->kind != RTC::Media::Kind::DEPTH)
			{
				continue;
			}

			if (rtpReceiver->GetTransport() != transport)
				continue;

			rtpReceiver->RequestFullFrame();
		}
	}

	void Peer::OnRtpReceiverParameters(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		auto rtpParameters = rtpReceiver->GetParameters();

		// Remove unsupported codecs and their associated encodings.
		rtpParameters->ReduceCodecsAndEncodings(this->capabilities);

		// Remove unsupported header extensions.
		rtpParameters->ReduceHeaderExtensions(this->capabilities.headerExtensions);

		auto transport = rtpReceiver->GetTransport();

		// NOTE: This may throw.
		if (transport != nullptr)
			transport->AddRtpReceiver(rtpReceiver);
	}

	void Peer::OnRtpReceiverParametersDone(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// Notify the listener (Room).
		this->listener->OnPeerRtpReceiverParameters(this, rtpReceiver);
	}

	void Peer::OnRtpPacket(RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->OnPeerRtpPacket(this, rtpReceiver, packet);
	}

	void Peer::OnTransportRtcpPacket(RTC::Transport* transport, RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		while (packet != nullptr)
		{
			switch (packet->GetType())
			{
				/* RTCP coming from a remote receiver which must be forwarded to the corresponding remote
				 * sender. */

				case RTCP::Type::RR:
				{
					auto* rr = dynamic_cast<RTCP::ReceiverReportPacket*>(packet);
					auto it  = rr->Begin();

					for (; it != rr->End(); ++it)
					{
						auto& report    = (*it);
						auto* rtpSender = this->GetRtpSender(report->GetSsrc());

						if (rtpSender != nullptr)
						{
							this->listener->OnPeerRtcpReceiverReport(this, rtpSender, report);
						}
						else
						{
							MS_WARN_TAG(
							    rtcp,
							    "no RtpSender found for received Receiver Report [ssrc:%" PRIu32 "]",
							    report->GetSsrc());
						}
					}

					break;
				}

				case RTCP::Type::PSFB:
				{
					auto* feedback = dynamic_cast<RTCP::FeedbackPsPacket*>(packet);

					switch (feedback->GetMessageType())
					{
						case RTCP::FeedbackPs::MessageType::AFB:
						{
							auto* afb = dynamic_cast<RTCP::FeedbackPsAfbPacket*>(feedback);

							if (afb->GetApplication() == RTCP::FeedbackPsAfbPacket::Application::REMB)
								break;
						}

						// [[fallthrough]]; (C++17)
						case RTCP::FeedbackPs::MessageType::PLI:
						case RTCP::FeedbackPs::MessageType::SLI:
						case RTCP::FeedbackPs::MessageType::RPSI:
						case RTCP::FeedbackPs::MessageType::FIR:
						{
							auto* rtpSender = this->GetRtpSender(feedback->GetMediaSsrc());

							if (rtpSender != nullptr)
							{
								// If the RtpSender is not active, drop the packet.
								if (!rtpSender->GetActive())
									break;

								if (feedback->GetMessageType() == RTCP::FeedbackPs::MessageType::PLI)
								{
									MS_DEBUG_TAG(rtx, "PLI received [media ssrc:%" PRIu32 "]", feedback->GetMediaSsrc());
								}

								this->listener->OnPeerRtcpFeedback(this, rtpSender, feedback);
							}
							else
							{
								MS_WARN_TAG(
								    rtcp,
								    "no RtpSender found for received %s Feedback packet "
								    "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
								    RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
								    feedback->GetMediaSsrc(),
								    feedback->GetMediaSsrc());
							}

							break;
						}

						case RTCP::FeedbackPs::MessageType::TSTR:
						case RTCP::FeedbackPs::MessageType::TSTN:
						case RTCP::FeedbackPs::MessageType::VBCM:
						case RTCP::FeedbackPs::MessageType::PSLEI:
						case RTCP::FeedbackPs::MessageType::ROI:
						case RTCP::FeedbackPs::MessageType::EXT:
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

					switch (feedback->GetMessageType())
					{
						case RTCP::FeedbackRtp::MessageType::NACK:
						{
							auto* rtpSender = this->GetRtpSender(feedback->GetMediaSsrc());

							if (rtpSender != nullptr)
							{
								auto* nackPacket = dynamic_cast<RTC::RTCP::FeedbackRtpNackPacket*>(packet);

								rtpSender->ReceiveNack(nackPacket);
							}
							else
							{
								MS_WARN_TAG(
								    rtcp,
								    "no RtpSender found for received NACK Feedback packet "
								    "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
								    feedback->GetMediaSsrc(),
								    feedback->GetMediaSsrc());
							}

							break;
						}

						case RTCP::FeedbackRtp::MessageType::TMMBR:
						case RTCP::FeedbackRtp::MessageType::TMMBN:
						case RTCP::FeedbackRtp::MessageType::SR_REQ:
						case RTCP::FeedbackRtp::MessageType::RAMS:
						case RTCP::FeedbackRtp::MessageType::TLLEI:
						case RTCP::FeedbackRtp::MessageType::ECN:
						case RTCP::FeedbackRtp::MessageType::PS:
						case RTCP::FeedbackRtp::MessageType::EXT:
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

				/* RTCP coming from a remote sender which must be forwarded to the corresponding remote
				 * receivers. */

				case RTCP::Type::SR:
				{
					auto* sr = dynamic_cast<RTCP::SenderReportPacket*>(packet);
					auto it  = sr->Begin();

					// Even if Sender Report packet can only contain one report..
					for (; it != sr->End(); ++it)
					{
						auto& report = (*it);
						// Get the receiver associated to the SSRC indicated in the report.
						auto* rtpReceiver = transport->GetRtpReceiver(report->GetSsrc());

						if (rtpReceiver != nullptr)
						{
							this->listener->OnPeerRtcpSenderReport(this, rtpReceiver, report);
						}
						else
						{
							MS_WARN_TAG(
							    rtcp,
							    "no RtpReceiver found for received Sender Report [ssrc:%" PRIu32 "]",
							    report->GetSsrc());
						}
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
						// Get the receiver associated to the SSRC indicated in the chunk.
						auto* rtpReceiver = transport->GetRtpReceiver(chunk->GetSsrc());

						if (rtpReceiver == nullptr)
						{
							MS_WARN_TAG(
							    rtcp, "no RtpReceiver for received SDES chunk [ssrc:%" PRIu32 "]", chunk->GetSsrc());
						}
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

			packet = packet->GetNext();
		}
	}

	void Peer::OnRtpReceiverClosed(const RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// We must remove the closed RtpReceiver from the Transports holding it.
		for (auto& kv : this->transports)
		{
			auto transport = kv.second;

			transport->RemoveRtpReceiver(rtpReceiver);
		}

		// Remove from the map.
		this->rtpReceivers.erase(rtpReceiver->rtpReceiverId);

		// Notify the listener (Room) so it can remove this RtpReceiver from its map.
		this->listener->OnPeerRtpReceiverClosed(this, rtpReceiver);
	}

	void Peer::OnRtpSenderClosed(RTC::RtpSender* rtpSender)
	{
		MS_TRACE();

		// Remove from the map.
		this->rtpSenders.erase(rtpSender->rtpSenderId);

		// Notify the listener (Room) so it can remove this RtpSender from its map.
		this->listener->OnPeerRtpSenderClosed(this, rtpSender);
	}

	void Peer::OnRtpSenderFullFrameRequired(RTC::RtpSender* rtpSender)
	{
		MS_TRACE();

		this->listener->OnFullFrameRequired(this, rtpSender);
	}

	void Peer::OnTimer(Timer* /*timer*/)
	{
		uint64_t interval = RTC::RTCP::MaxVideoIntervalMs;
		uint32_t now      = DepLibUV::GetTime();

		this->SendRtcp(now);

		// Recalculate next RTCP interval.
		if (!this->rtpSenders.empty())
		{
			// Transmission rate in kbps.
			uint32_t rate = 0;

			// Get the RTP sending rate.
			for (auto& kv : this->rtpSenders)
			{
				auto* rtpSender = kv.second;

				rate += rtpSender->GetTransmissionRate(now) / 1000;
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
		this->timer->Start(interval);
	}
} // namespace RTC
