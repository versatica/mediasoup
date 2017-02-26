#define MS_CLASS "RTC::Peer"
// #define MS_LOG_DEV

#include "RTC/Peer.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "MediaSoupError.hpp"
#include "Logger.hpp"
#include "DepLibUV.hpp"

namespace RTC
{
	/* Class variables. */

	uint8_t Peer::rtcpBuffer[MS_RTCP_BUFFER_SIZE];

	/* Instance methods. */

	Peer::Peer(Listener* listener, Channel::Notifier* notifier, uint32_t peerId, std::string& peerName) :
		peerId(peerId),
		peerName(peerName),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();

		this->timer = new Timer(this);

		// Start the RTCP timer.
		this->timer->Start(uint64_t(RTC::RTCP::MAX_VIDEO_INTERVAL_MS / 2));
	}

	Peer::~Peer()
	{
		MS_TRACE();

		// Close the RTCP timer.
		this->timer->Close();
	}

	void Peer::Close()
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");

		Json::Value event_data(Json::objectValue);

		// Close all the RtpReceivers.
		for (auto it = this->rtpReceivers.begin(); it != this->rtpReceivers.end();)
		{
			RTC::RtpReceiver* rtpReceiver = it->second;

			it = this->rtpReceivers.erase(it);
			rtpReceiver->Close();
		}

		// Close all the RtpSenders.
		for (auto it = this->rtpSenders.begin(); it != this->rtpSenders.end();)
		{
			RTC::RtpSender* rtpSender = it->second;

			it = this->rtpSenders.erase(it);
			rtpSender->Close();
		}

		// Close all the Transports.
		// NOTE: It is critical to close Transports after RtpReceivers/RtpSenders
		// because RtcReceiver.Close() fires an event in the Transport.
		for (auto it = this->transports.begin(); it != this->transports.end();)
		{
			RTC::Transport* transport = it->second;

			it = this->transports.erase(it);
			transport->Close();
		}

		// Notify.
		event_data[k_class] = "Peer";
		this->notifier->Emit(this->peerId, "close", event_data);

		// Notify the listener.
		this->listener->onPeerClosed(this);

		delete this;
	}

	Json::Value Peer::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_peerId("peerId");
		static const Json::StaticString k_peerName("peerName");
		static const Json::StaticString k_capabilities("capabilities");
		static const Json::StaticString k_transports("transports");
		static const Json::StaticString k_rtpReceivers("rtpReceivers");
		static const Json::StaticString k_rtpSenders("rtpSenders");

		Json::Value json(Json::objectValue);
		Json::Value json_transports(Json::arrayValue);
		Json::Value json_rtpReceivers(Json::arrayValue);
		Json::Value json_rtpSenders(Json::arrayValue);

		// Add `peerId`.
		json[k_peerId] = (Json::UInt)this->peerId;

		// Add `peerName`.
		json[k_peerName] = this->peerName;

		// Add `capabilities`.
		if (this->hasCapabilities)
			json[k_capabilities] = this->capabilities.toJson();

		// Add `transports`.
		for (auto& kv : this->transports)
		{
			RTC::Transport* transport = kv.second;

			json_transports.append(transport->toJson());
		}
		json[k_transports] = json_transports;

		// Add `rtpReceivers`.
		for (auto& kv : this->rtpReceivers)
		{
			RTC::RtpReceiver* rtpReceiver = kv.second;

			json_rtpReceivers.append(rtpReceiver->toJson());
		}
		json[k_rtpReceivers] = json_rtpReceivers;

		// Add `rtpSenders`.
		for (auto& kv : this->rtpSenders)
		{
			RTC::RtpSender* rtpSender = kv.second;

			json_rtpSenders.append(rtpSender->toJson());
		}
		json[k_rtpSenders] = json_rtpSenders;

		return json;
	}

	void Peer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::peer_close:
			{
				#ifdef MS_LOG_DEV
				uint32_t peerId = this->peerId;
				#endif

				Close();

				MS_DEBUG_DEV("Peer closed [peerId:%" PRIu32 "]", peerId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::peer_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::peer_setCapabilities:
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
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				this->hasCapabilities = true;

				// Notify the listener (Room) who will remove capabilities to make them
				// a subset of the room capabilities.
				this->listener->onPeerCapabilities(this, std::addressof(this->capabilities));

				Json::Value data = this->capabilities.toJson();

				// NOTE: We accept the request *after* calling onPeerCapabilities(). This
				// guarantees that the Peer will receive a 'newrtpsender' event for all its
				// associated RtpSenders *before* the setCapabilities() Promise resolves.
				// In other words, at the time setCapabilities() resolves, the Peer already
				// has set all its current RtpSenders.
				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::peer_createTransport:
			{
				RTC::Transport* transport;
				uint32_t transportId;

				try
				{
					transport = GetTransportFromRequest(request, &transportId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (transport)
				{
					request->Reject("Transport already exists");
					return;
				}

				try
				{
					transport = new RTC::Transport(this, this->notifier, transportId, request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				this->transports[transportId] = transport;

				MS_DEBUG_DEV("Transport created [transportId:%" PRIu32 "]", transportId);

				auto data = transport->toJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::peer_createRtpReceiver:
			{
				static const Json::StaticString k_kind("kind");

				RTC::RtpReceiver* rtpReceiver;
				RTC::Transport* transport = nullptr;
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
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (rtpReceiver)
				{
					request->Reject("RtpReceiver already exists");
					return;
				}

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!transport)
				{
					request->Reject("Transport does not exist");
					return;
				}

				// `kind` is mandatory.

				if (!request->data[k_kind].isString())
					MS_THROW_ERROR("missing kind");

				std::string kind = request->data[k_kind].asString();

				// Create a RtpReceiver instance.
				try
				{
					rtpReceiver = new RTC::RtpReceiver(this, this->notifier, rtpReceiverId, RTC::Media::GetKind(kind));
				}
				catch (const MediaSoupError &error)
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

			case Channel::Request::MethodId::transport_close:
			case Channel::Request::MethodId::transport_dump:
			case Channel::Request::MethodId::transport_setRemoteDtlsParameters:
			{
				RTC::Transport* transport;

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!transport)
				{
					request->Reject("Transport does not exist");
					return;
				}

				transport->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_close:
			case Channel::Request::MethodId::rtpReceiver_dump:
			case Channel::Request::MethodId::rtpReceiver_receive:
			case Channel::Request::MethodId::rtpReceiver_setRtpRawEvent:
			case Channel::Request::MethodId::rtpReceiver_setRtpObjectEvent:
			{
				RTC::RtpReceiver* rtpReceiver;

				try
				{
					rtpReceiver = GetRtpReceiverFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!rtpReceiver)
				{
					request->Reject("RtpReceiver does not exist");
					return;
				}

				rtpReceiver->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::rtpSender_dump:
			{
				RTC::RtpSender* rtpSender;

				try
				{
					rtpSender = GetRtpSenderFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!rtpSender)
				{
					request->Reject("RtpSender does not exist");
					return;
				}

				rtpSender->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::rtpSender_setTransport:
			{
				RTC::RtpSender* rtpSender;

				try
				{
					rtpSender = GetRtpSenderFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!rtpSender)
				{
					request->Reject("RtpSender does not exist");
					return;
				}

				RTC::Transport* transport;

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!transport)
				{
					request->Reject("Transport does not exist");
					return;
				}

				rtpSender->SetTransport(transport);

				request->Accept();

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void Peer::AddRtpSender(RTC::RtpSender* rtpSender, std::string& peerName, RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");
		static const Json::StaticString k_peerName("peerName");

		MS_ASSERT(this->rtpSenders.find(rtpSender->rtpSenderId) == this->rtpSenders.end(),
			"given RtpSender already exists in this Peer");

		// Provide the RtpSender with peer's capabilities.
		rtpSender->SetPeerCapabilities(std::addressof(this->capabilities));

		// Provide the RtpSender with the received RTP parameters.
		rtpSender->Send(rtpParameters);

		// Store it.
		this->rtpSenders[rtpSender->rtpSenderId] = rtpSender;

		// Notify.
		Json::Value event_data = rtpSender->toJson();

		event_data[k_class] = "Peer";
		event_data[k_peerName] = peerName;

		this->notifier->Emit(this->peerId, "newrtpsender", event_data);
	}

	RTC::RtpSender* Peer::GetRtpSender(uint32_t ssrc)
	{
		MS_TRACE();

		for (auto it = this->rtpSenders.begin(); it != this->rtpSenders.end(); ++it)
		{
			auto rtpSender = it->second;
			auto rtpParameters = rtpSender->GetParameters();

			if (!rtpParameters)
				continue;

			for (auto it2 = rtpParameters->encodings.begin(); it2 != rtpParameters->encodings.end(); ++it2)
			{
				auto& encoding = *it2;

				if (encoding.ssrc == ssrc)
					return rtpSender;
				else if (encoding.hasFec && encoding.fec.ssrc == ssrc)
					return rtpSender;
				else if (encoding.hasRtx && encoding.rtx.ssrc == ssrc)
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

		for (auto it = this->transports.begin(); it != this->transports.end(); ++it)
		{
			std::unique_ptr<RTC::RTCP::CompoundPacket> packet(new RTC::RTCP::CompoundPacket());
			RTC::Transport* transport = it->second;

			for (auto it = this->rtpSenders.begin(); it != this->rtpSenders.end(); ++it)
			{
				RTC::RtpSender* rtpSender = it->second;

				if (rtpSender->GetTransport() != transport)
					continue;

				rtpSender->GetRtcp(packet.get(), now);

				// Send one RTCP compound packet per sender report.
				if (packet->GetSenderReportCount())
				{
					// Ensure that the RTCP packet fits into the RTCP buffer.
					if (packet->GetSize() > MS_RTCP_BUFFER_SIZE)
					{
						MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());
						return;
					}

					packet->Serialize(Peer::rtcpBuffer);
					transport->SendRtcpCompoundPacket(packet.get());

					// reset the Compound packet.
					packet.reset(new RTC::RTCP::CompoundPacket());
				}
			}

			for (auto it = this->rtpReceivers.begin(); it != this->rtpReceivers.end(); ++it)
			{
				RTC::RtpReceiver* rtpReceiver = it->second;

				if (rtpReceiver->GetTransport() != transport)
					continue;

				rtpReceiver->GetRtcp(packet.get(), now);
			}

			// Send one RTCP compound with all receiver reports.
			if (packet->GetReceiverReportCount())
			{
				// Ensure that the RTCP packet fits into the RTCP buffer.
				if (packet->GetSize() > MS_RTCP_BUFFER_SIZE)
				{
					MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)", packet->GetSize());
					return;
				}

				packet->Serialize(Peer::rtcpBuffer);
				transport->SendRtcpCompoundPacket(packet.get());
			}
		}
	}

	RTC::Transport* Peer::GetTransportFromRequest(Channel::Request* request, uint32_t* transportId)
	{
		MS_TRACE();

		static const Json::StaticString k_transportId("transportId");

		auto json_transportId = request->internal[k_transportId];

		if (!json_transportId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.transportId");

		if (transportId)
			*transportId = json_transportId.asUInt();

		auto it = this->transports.find(json_transportId.asUInt());
		if (it != this->transports.end())
		{
			RTC::Transport* transport = it->second;

			return transport;
		}
		else
		{
			return nullptr;
		}
	}

	RTC::RtpReceiver* Peer::GetRtpReceiverFromRequest(Channel::Request* request, uint32_t* rtpReceiverId)
	{
		MS_TRACE();

		static const Json::StaticString k_rtpReceiverId("rtpReceiverId");

		auto json_rtpReceiverId = request->internal[k_rtpReceiverId];

		if (!json_rtpReceiverId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.rtpReceiverId");

		if (rtpReceiverId)
			*rtpReceiverId = json_rtpReceiverId.asUInt();

		auto it = this->rtpReceivers.find(json_rtpReceiverId.asUInt());
		if (it != this->rtpReceivers.end())
		{
			RTC::RtpReceiver* rtpReceiver = it->second;

			return rtpReceiver;
		}
		else
		{
			return nullptr;
		}
	}

	RTC::RtpSender* Peer::GetRtpSenderFromRequest(Channel::Request* request, uint32_t* rtpSenderId)
	{
		MS_TRACE();

		static const Json::StaticString k_rtpSenderId("rtpSenderId");

		auto json_rtpSenderId = request->internal[k_rtpSenderId];

		if (!json_rtpSenderId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.rtpSenderId");

		if (rtpSenderId)
			*rtpSenderId = json_rtpSenderId.asUInt();

		auto it = this->rtpSenders.find(json_rtpSenderId.asUInt());
		if (it != this->rtpSenders.end())
		{
			RTC::RtpSender* rtpSender = it->second;

			return rtpSender;
		}
		else
		{
			return nullptr;
		}
	}

	void Peer::onTransportClosed(RTC::Transport* transport)
	{
		MS_TRACE();

		// Must remove the closed Transport from all the RtpReceivers holding it.
		for (auto& kv : this->rtpReceivers)
		{
			RTC::RtpReceiver* rtpReceiver = kv.second;

			rtpReceiver->RemoveTransport(transport);
		}

		// Must also unset this Transport from all the RtpSenders using it.
		for (auto& kv : this->rtpSenders)
		{
			RTC::RtpSender* rtpSender = kv.second;

			rtpSender->RemoveTransport(transport);
		}

		this->transports.erase(transport->transportId);
	}

	void Peer::onRtpReceiverParameters(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		auto rtpParameters = rtpReceiver->GetParameters();

		// Remove unsupported codecs and their associated encodings.
		rtpParameters->ReduceCodecsAndEncodings(this->capabilities);

		// Remove unsupported header extensions.
		rtpParameters->ReduceHeaderExtensions(this->capabilities.headerExtensions);

		auto transport = rtpReceiver->GetTransport();

		// NOTE: This may throw.
		if (transport)
			transport->AddRtpReceiver(rtpReceiver);
	}

	void Peer::onRtpReceiverParametersDone(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// Notify the listener (Room).
		this->listener->onPeerRtpReceiverParameters(this, rtpReceiver);
	}

	void Peer::onRtpPacket(RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->onPeerRtpPacket(this, rtpReceiver, packet);
	}

	void Peer::onTransportRtcpPacket(RTC::Transport* transport, RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		while (packet)
		{
			switch (packet->GetType())
			{
				/* RTCP coming from a remote receiver which must be forwarded to the corresponding remote sender. */

				case RTCP::Type::RR:
				{
					RTCP::ReceiverReportPacket* rr = static_cast<RTCP::ReceiverReportPacket*>(packet);
					RTCP::ReceiverReportPacket::Iterator it = rr->Begin();

					for (; it != rr->End(); ++it)
					{
						auto& report = (*it);
						RTC::RtpSender* rtpSender = this->GetRtpSender(report->GetSsrc());

						if (rtpSender)
						{
							this->listener->onPeerRtcpReceiverReport(this, rtpSender, report);
						}
						else
						{
							MS_WARN_TAG(rtcp, "no RtpSender found while procesing a Receiver Report [ssrc:%" PRIu32 "]", report->GetSsrc());
						}
					}

					break;
				}

				case RTCP::Type::PSFB:
				{
					RTCP::FeedbackPsPacket* feedback = static_cast<RTCP::FeedbackPsPacket*>(packet);

					switch (feedback->GetMessageType())
					{
						case RTCP::FeedbackPs::PLI:
						case RTCP::FeedbackPs::SLI:
						case RTCP::FeedbackPs::RPSI:
						case RTCP::FeedbackPs::FIR:
						case RTCP::FeedbackPs::AFB:
						{
							RTC::RtpSender* rtpSender = this->GetRtpSender(feedback->GetMediaSsrc());

							if (rtpSender)
							{
								this->listener->onPeerRtcpFeedback(this, rtpSender, feedback);
							}
							else
							{
								MS_WARN_TAG(rtcp, "no RtpSender found while procesing a Feedback packet '%s' [sender_ssrc:%" PRIu32 ", media_ssrc:%" PRIu32 "]",
									RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(), feedback->GetMediaSsrc(), feedback->GetMediaSsrc());
							}

							break;
						}

						case RTCP::FeedbackPs::TSTR:
						case RTCP::FeedbackPs::TSTN:
						case RTCP::FeedbackPs::VBCM:
						case RTCP::FeedbackPs::PSLEI:
						case RTCP::FeedbackPs::ROI:
						case RTCP::FeedbackPs::EXT:
						default:
						{
							MS_WARN_TAG(rtcp, "ignoring Feedback packet '%s' [sender_ssrc:%" PRIu32 ", media_ssrc:%" PRIu32 "]",
								RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(), feedback->GetMediaSsrc(), feedback->GetMediaSsrc());

							break;
						}
					}

					break;
				}

				case RTCP::Type::RTPFB:
				{
					RTCP::FeedbackRtpPacket* feedback = static_cast<RTCP::FeedbackRtpPacket*>(packet);

					switch (feedback->GetMessageType())
					{
						case RTCP::FeedbackRtp::NACK:
						{
							RTC::RtpSender* rtpSender = this->GetRtpSender(feedback->GetMediaSsrc());

							if (rtpSender)
							{
								RTC::RTCP::FeedbackRtpNackPacket* nackPacket = static_cast<RTC::RTCP::FeedbackRtpNackPacket*>(packet);

								rtpSender->ReceiveNack(nackPacket);
							}
							else
							{
								MS_WARN_TAG(rtcp, "no RtpSender found while procesing a NACK packet '%s' [sender_ssrc:%" PRIu32 ", media_ssrc:%" PRIu32 "]",
									RTCP::FeedbackRtpPacket::MessageType2String(feedback->GetMessageType()).c_str(), feedback->GetMediaSsrc(), feedback->GetMediaSsrc());
							}

							break;
						}

						case RTCP::FeedbackRtp::TMMBR:
						case RTCP::FeedbackRtp::TMMBN:
						case RTCP::FeedbackRtp::SR_REQ:
						case RTCP::FeedbackRtp::RAMS:
						case RTCP::FeedbackRtp::TLLEI:
						case RTCP::FeedbackRtp::ECN:
						case RTCP::FeedbackRtp::PS:
						case RTCP::FeedbackRtp::EXT:
						default:
						{
							MS_WARN_TAG(rtcp, "ignoring unsupported Feedback packet '%s' [sender_ssrc:%" PRIu32 ", media_ssrc:%" PRIu32 "]",
								RTCP::FeedbackRtpPacket::MessageType2String(feedback->GetMessageType()).c_str(), feedback->GetMediaSsrc(), feedback->GetMediaSsrc());

							break;
						}
					}

					break;
				}

				/* RTCP coming from a remote sender which must be forwarded to the corresponding remote receivers. */

				case RTCP::Type::SR:
				{
					RTCP::SenderReportPacket* sr = static_cast<RTCP::SenderReportPacket*>(packet);
					RTCP::SenderReportPacket::Iterator it = sr->Begin();

					// Even if Sender Report packet can only contain one report..
					for (; it != sr->End(); ++it)
					{
						auto& report = (*it);
						// Get the receiver associated to the SSRC indicated in the report.
						RTC::RtpReceiver* rtpReceiver = transport->GetRtpReceiver(report->GetSsrc());

						if (rtpReceiver)
						{
							this->listener->onPeerRtcpSenderReport(this, rtpReceiver, report);
						}
						else
						{
							MS_WARN_TAG(rtcp, "no RtpReceiver found while procesing a Sender Report [ssrc:%" PRIu32 "]",
								report->GetSsrc());
						}
					}

					break;
				}

				case RTCP::Type::SDES:
				{
					RTCP::SdesPacket* sdes = static_cast<RTCP::SdesPacket*>(packet);
					RTCP::SdesPacket::Iterator it = sdes->Begin();

					for (; it != sdes->End(); ++it)
					{
						auto& chunk = (*it);
						// Get the receiver associated to the SSRC indicated in the chunk.
						RTC::RtpReceiver* rtpReceiver = transport->GetRtpReceiver(chunk->GetSsrc());

						if (!rtpReceiver)
						{
							MS_WARN_TAG(rtcp, "no RtpReceiver found while procesing a SDES chunk [ssrc:%" PRIu32 "]",
								chunk->GetSsrc());
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
					MS_WARN_TAG(rtcp, "unhandled RTCP type received [type:%" PRIu8 "]",
						(uint8_t)packet->GetType());
				}
			}

			packet = packet->GetNext();
		}
	}

	void Peer::onRtpReceiverClosed(RTC::RtpReceiver* rtpReceiver)
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
		this->listener->onPeerRtpReceiverClosed(this, rtpReceiver);
	}

	void Peer::onRtpSenderClosed(RTC::RtpSender* rtpSender)
	{
		MS_TRACE();

		// Remove from the map.
		this->rtpSenders.erase(rtpSender->rtpSenderId);

		// Notify the listener (Room) so it can remove this RtpSender from its map.
		this->listener->onPeerRtpSenderClosed(this, rtpSender);
	}

	void Peer::onTimer(Timer* timer)
	{
		uint64_t interval = RTC::RTCP::MAX_VIDEO_INTERVAL_MS;
		uint32_t now = DepLibUV::GetTime();

		this->SendRtcp(now);

		// Recalculate next RTCP interval.
		if (this->rtpSenders.size())
		{
			// Transmission rate in kbps.
			uint32_t rate = 0;

			// Get the RTP sending rate.
			for (auto& kv : this->rtpSenders)
			{
				RTC::RtpSender* rtpSender = kv.second;

				rate += rtpSender->GetTransmissionRate(now) / 1000;
			}

			// Calculate bandwidth: 360 / transmission bandwidth in kbit/s
			if (rate)
				interval = 360000 / rate;

			if (interval > RTC::RTCP::MAX_VIDEO_INTERVAL_MS)
				interval = RTC::RTCP::MAX_VIDEO_INTERVAL_MS;
		}

		/*
		 * The interval between RTCP packets is varied randomly over the range
		 * [0.5,1.5] times the calculated interval to avoid unintended synchronization
		 * of all participants.
		 */
		interval *= static_cast<float>(Utils::Crypto::GetRandomUInt(5, 15)) / 10;

		this->timer->Start(interval);
	}
}
