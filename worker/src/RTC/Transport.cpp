#define MS_CLASS "RTC::Transport"
// #define MS_LOG_DEV

#include "RTC/Transport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/PipeConsumer.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/SimpleConsumer.hpp"
#include "RTC/SimulcastConsumer.hpp"

namespace RTC
{
	/* Instance methods. */

	Transport::Transport(const std::string& id, Listener* listener) : id(id), listener(listener)
	{
		MS_TRACE();

		// Create the RTCP timer.
		this->rtcpTimer = new Timer(this);
	}

	Transport::~Transport()
	{
		MS_TRACE();

		// The destructor must delete and clear everything silently.

		// Delete all Producers.
		for (auto& kv : this->mapProducers)
		{
			auto* producer = kv.second;

			delete producer;
		}
		this->mapProducers.clear();

		// Delete all Consumers.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			delete consumer;
		}
		this->mapConsumers.clear();
		this->mapSsrcConsumer.clear();

		// Delete the RTCP timer.
		delete this->rtcpTimer;
	}

	void Transport::CloseProducersAndConsumers()
	{
		MS_TRACE();

		// This method is called by the Router and must notify him about all Producers
		// and Consumers that we are gonna close.
		//
		// The caller is supposed to delete this Transport instance after calling
		// this method.

		// Close all Producers.
		for (auto& kv : this->mapProducers)
		{
			auto* producer = kv.second;

			// Notify the listener.
			this->listener->OnTransportProducerClosed(this, producer);

			delete producer;
		}
		this->mapProducers.clear();

		// Delete all Consumers.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			// Notify the listener.
			this->listener->OnTransportConsumerClosed(this, consumer);

			delete consumer;
		}
		this->mapConsumers.clear();
		this->mapSsrcConsumer.clear();
	}

	void Transport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add producerIds.
		jsonObject["producerIds"] = json::array();
		auto jsonProducerIdsIt    = jsonObject.find("producerIds");

		for (auto& kv : this->mapProducers)
		{
			auto& producerId = kv.first;

			jsonProducerIdsIt->emplace_back(producerId);
		}

		// Add consumerIds.
		jsonObject["consumerIds"] = json::array();
		auto jsonConsumerIdsIt    = jsonObject.find("consumerIds");

		for (auto& kv : this->mapConsumers)
		{
			auto& consumerId = kv.first;

			jsonConsumerIdsIt->emplace_back(consumerId);
		}

		// Add mapSsrcConsumerId.
		jsonObject["mapSsrcConsumerId"] = json::object();
		auto jsonMapSsrcConsumerId      = jsonObject.find("mapSsrcConsumerId");

		for (auto& kv : this->mapSsrcConsumer)
		{
			auto ssrc      = kv.first;
			auto* consumer = kv.second;

			(*jsonMapSsrcConsumerId)[std::to_string(ssrc)] = consumer->id;
		}
	}

	void Transport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_DUMP:
			{
				json data(json::object());

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_GET_STATS:
			{
				json data(json::array());

				FillJsonStats(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_PRODUCE:
			{
				std::string producerId;

				// This may throw.
				SetNewProducerIdFromRequest(request, producerId);

				// This may throw.
				auto* producer = new RTC::Producer(producerId, this, request->data);

				// Insert the Producer into the RtpListener.
				// This may throw. If so, delete the Producer and throw.
				try
				{
					this->rtpListener.AddProducer(producer);
				}
				catch (const MediaSoupError& error)
				{
					delete producer;

					throw;
				}

				// Notify the listener.
				// This may throw if a Producer with same id already exists.
				try
				{
					this->listener->OnTransportNewProducer(this, producer);
				}
				catch (const MediaSoupError& error)
				{
					delete producer;

					throw;
				}

				// Insert into the map.
				this->mapProducers[producerId] = producer;

				MS_DEBUG_DEV("Producer created [producerId:%s]", producerId.c_str());

				// Take the transport related RTP header extensions of the Producer and
				// add them to the Transport.
				// NOTE: Producer::GetRtpHeaderExtensionIds() returns the original
				// header extension ids of the Producer (and not their mapped values).
				auto& producerRtpHeaderExtensionIds = producer->GetRtpHeaderExtensionIds();

				if (producerRtpHeaderExtensionIds.mid != 0u)
					this->rtpHeaderExtensionIds.mid = producerRtpHeaderExtensionIds.mid;

				if (producerRtpHeaderExtensionIds.rid != 0u)
					this->rtpHeaderExtensionIds.rid = producerRtpHeaderExtensionIds.rid;

				if (producerRtpHeaderExtensionIds.rrid != 0u)
					this->rtpHeaderExtensionIds.rrid = producerRtpHeaderExtensionIds.rrid;

				if (producerRtpHeaderExtensionIds.absSendTime != 0u)
					this->rtpHeaderExtensionIds.absSendTime = producerRtpHeaderExtensionIds.absSendTime;

				// Create status response.
				json data(json::object());

				data["type"] = RTC::RtpParameters::GetTypeString(producer->GetType());

				request->Accept(data);

				// Tell the subclass.
				UserOnNewProducer(producer);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_CONSUME:
			{
				auto jsonProducerIdIt = request->internal.find("producerId");

				if (jsonProducerIdIt == request->internal.end() || !jsonProducerIdIt->is_string())
					MS_THROW_ERROR("request has no internal.producerId");

				std::string producerId = jsonProducerIdIt->get<std::string>();
				std::string consumerId;

				// This may throw.
				SetNewConsumerIdFromRequest(request, consumerId);

				// Get type.
				auto jsonTypeIt = request->data.find("type");

				if (jsonTypeIt == request->data.end() || !jsonTypeIt->is_string())
					MS_THROW_TYPE_ERROR("missing type");

				// This may throw.
				auto type = RTC::RtpParameters::GetType(jsonTypeIt->get<std::string>());

				RTC::Consumer* consumer{ nullptr };

				switch (type)
				{
					case RTC::RtpParameters::Type::NONE:
					{
						MS_THROW_TYPE_ERROR("invalid type 'none'");

						break;
					}

					case RTC::RtpParameters::Type::SIMPLE:
					{
						// This may throw.
						consumer = new RTC::SimpleConsumer(consumerId, this, request->data);

						break;
					}

					case RTC::RtpParameters::Type::SIMULCAST:
					{
						// This may throw.
						consumer = new RTC::SimulcastConsumer(consumerId, this, request->data);

						break;
					}

					case RTC::RtpParameters::Type::SVC:
					{
						MS_THROW_TYPE_ERROR("not yet implemented type 'svc'");

						break;
					}

					case RTC::RtpParameters::Type::PIPE:
					{
						// This may throw.
						consumer = new RTC::PipeConsumer(consumerId, this, request->data);

						break;
					}
				}

				// Notify the listener.
				// This may throw if no Producer is found.
				try
				{
					this->listener->OnTransportNewConsumer(this, consumer, producerId);
				}
				catch (const MediaSoupError& error)
				{
					delete consumer;

					throw;
				}

				// Insert into the maps.
				this->mapConsumers[consumerId] = consumer;

				for (auto ssrc : consumer->GetMediaSsrcs())
				{
					this->mapSsrcConsumer[ssrc] = consumer;
				}

				MS_DEBUG_DEV(
				  "Consumer created [consumerId:%s, producerId:%s]", consumerId.c_str(), producerId.c_str());

				// Create status response.
				json data(json::object());

				data["paused"]         = consumer->IsPaused();
				data["producerPaused"] = consumer->IsProducerPaused();

				consumer->FillJsonScore(data["score"]);

				request->Accept(data);

				// Tell the subclass.
				UserOnNewConsumer(consumer);

				// TODO: Update with new BWE API.
				//
				// If the Transport is already connected tell the new Consumer to ask for bandwidth.
				if (IsConnected())
					consumer->UseBandwidth(0);

				break;
			}

			case Channel::Request::MethodId::PRODUCER_CLOSE:
			{
				// This may throw.
				RTC::Producer* producer = GetProducerFromRequest(request);

				// Remove it from the RtpListener.
				this->rtpListener.RemoveProducer(producer);

				// Remove it from the map.
				this->mapProducers.erase(producer->id);

				// Notify the listener.
				this->listener->OnTransportProducerClosed(this, producer);

				MS_DEBUG_DEV("Producer closed [producerId:%s]", producer->id.c_str());

				// Delete it.
				delete producer;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_CLOSE:
			{
				// This may throw.
				RTC::Consumer* consumer = GetConsumerFromRequest(request);

				// Remove it from the maps.
				this->mapConsumers.erase(consumer->id);

				for (auto ssrc : consumer->GetMediaSsrcs())
				{
					this->mapSsrcConsumer.erase(ssrc);
				}

				// Notify the listener.
				this->listener->OnTransportConsumerClosed(this, consumer);

				MS_DEBUG_DEV("Consumer closed [consumerId:%s]", consumer->id.c_str());

				// Delete it.
				delete consumer;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_DUMP:
			case Channel::Request::MethodId::PRODUCER_GET_STATS:
			case Channel::Request::MethodId::PRODUCER_PAUSE:
			case Channel::Request::MethodId::PRODUCER_RESUME:
			{
				// This may throw.
				RTC::Producer* producer = GetProducerFromRequest(request);

				producer->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::CONSUMER_DUMP:
			case Channel::Request::MethodId::CONSUMER_GET_STATS:
			case Channel::Request::MethodId::CONSUMER_PAUSE:
			case Channel::Request::MethodId::CONSUMER_RESUME:
			case Channel::Request::MethodId::CONSUMER_SET_PREFERRED_LAYERS:
			case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
			{
				// This may throw.
				RTC::Consumer* consumer = GetConsumerFromRequest(request);

				consumer->HandleRequest(request);

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void Transport::Connected()
	{
		MS_TRACE();

		// Start the RTCP timer.
		this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));

		// Iterate all Consumers and tell them to use bandwidth.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			// TODO: Update with new BWE API.
			consumer->UseBandwidth(0);
		}
	}

	void Transport::Disconnected()
	{
		MS_TRACE();

		// Stop the RTCP timer.
		this->rtcpTimer->Stop();
	}

	void Transport::ReceiveRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		switch (packet->GetType())
		{
			case RTC::RTCP::Type::RR:
			{
				auto* rr = static_cast<RTC::RTCP::ReceiverReportPacket*>(packet);

				for (auto it = rr->Begin(); it != rr->End(); ++it)
				{
					auto& report   = (*it);
					auto* consumer = GetConsumerByMediaSsrc(report->GetSsrc());

					if (consumer == nullptr)
					{
						MS_DEBUG_TAG(
						  rtcp,
						  "no Consumer found for received Receiver Report [ssrc:%" PRIu32 "]",
						  report->GetSsrc());

						continue;
					}

					consumer->ReceiveRtcpReceiverReport(report);
				}

				break;
			}

			case RTC::RTCP::Type::PSFB:
			{
				auto* feedback = static_cast<RTC::RTCP::FeedbackPsPacket*>(packet);

				switch (feedback->GetMessageType())
				{
					case RTC::RTCP::FeedbackPs::MessageType::PLI:
					case RTC::RTCP::FeedbackPs::MessageType::FIR:
					{
						auto* consumer = GetConsumerByMediaSsrc(feedback->GetMediaSsrc());

						if (consumer == nullptr)
						{
							MS_DEBUG_TAG(
							  rtcp,
							  "no Consumer found for received %s Feedback packet "
							  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
							  RTC::RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
							  feedback->GetMediaSsrc(),
							  feedback->GetMediaSsrc());

							break;
						}

						MS_DEBUG_2TAGS(
						  rtcp,
						  rtx,
						  "%s received, requesting key frame for Consumer "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  RTC::RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
						  feedback->GetMediaSsrc(),
						  feedback->GetMediaSsrc());

						consumer->ReceiveKeyFrameRequest(feedback->GetMessageType());

						break;
					}

					case RTC::RTCP::FeedbackPs::MessageType::AFB:
					{
						auto* afb = static_cast<RTC::RTCP::FeedbackPsAfbPacket*>(feedback);

						// Store REMB info.
						if (afb->GetApplication() == RTC::RTCP::FeedbackPsAfbPacket::Application::REMB)
						{
							auto* remb = static_cast<RTC::RTCP::FeedbackPsRembPacket*>(afb);

							// Pass it to the subclass.
							UserOnRembFeedback(remb);

							break;
						}
						else
						{
							MS_DEBUG_TAG(
							  rtcp,
							  "ignoring unsupported %s Feedback PS AFB packet "
							  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
							  RTC::RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
							  feedback->GetMediaSsrc(),
							  feedback->GetMediaSsrc());

							break;
						}
					}

					default:
					{
						MS_DEBUG_TAG(
						  rtcp,
						  "ignoring unsupported %s Feedback packet "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  RTC::RTCP::FeedbackPsPacket::MessageType2String(feedback->GetMessageType()).c_str(),
						  feedback->GetMediaSsrc(),
						  feedback->GetMediaSsrc());
					}
				}

				break;
			}

			case RTC::RTCP::Type::RTPFB:
			{
				auto* feedback = static_cast<RTC::RTCP::FeedbackRtpPacket*>(packet);
				auto* consumer = GetConsumerByMediaSsrc(feedback->GetMediaSsrc());

				if (consumer == nullptr)
				{
					MS_DEBUG_TAG(
					  rtcp,
					  "no Consumer found for received Feedback packet "
					  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
					  feedback->GetMediaSsrc(),
					  feedback->GetMediaSsrc());

					break;
				}

				switch (feedback->GetMessageType())
				{
					case RTC::RTCP::FeedbackRtp::MessageType::NACK:
					{
						auto* nackPacket = static_cast<RTC::RTCP::FeedbackRtpNackPacket*>(packet);

						consumer->ReceiveNack(nackPacket);

						break;
					}

					default:
					{
						MS_DEBUG_TAG(
						  rtcp,
						  "ignoring unsupported %s Feedback packet "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  RTC::RTCP::FeedbackRtpPacket::MessageType2String(feedback->GetMessageType()).c_str(),
						  feedback->GetMediaSsrc(),
						  feedback->GetMediaSsrc());
					}
				}

				break;
			}

			case RTC::RTCP::Type::SR:
			{
				auto* sr = static_cast<RTC::RTCP::SenderReportPacket*>(packet);

				// Even if Sender Report packet can only contains one report...
				for (auto it = sr->Begin(); it != sr->End(); ++it)
				{
					auto& report = (*it);
					// Get the producer associated to the SSRC indicated in the report.
					auto* producer = this->rtpListener.GetProducer(report->GetSsrc());

					if (producer == nullptr)
					{
						MS_DEBUG_TAG(
						  rtcp,
						  "no Producer found for received Sender Report [ssrc:%" PRIu32 "]",
						  report->GetSsrc());

						continue;
					}

					producer->ReceiveRtcpSenderReport(report);
				}

				break;
			}

			case RTC::RTCP::Type::SDES:
			{
				auto* sdes = static_cast<RTC::RTCP::SdesPacket*>(packet);

				for (auto it = sdes->Begin(); it != sdes->End(); ++it)
				{
					auto& chunk = (*it);
					// Get the producer associated to the SSRC indicated in the report.
					auto* producer = this->rtpListener.GetProducer(chunk->GetSsrc());

					if (producer == nullptr)
					{
						MS_DEBUG_TAG(
						  rtcp, "no Producer for received SDES chunk [ssrc:%" PRIu32 "]", chunk->GetSsrc());

						continue;
					}
				}

				break;
			}

			case RTC::RTCP::Type::BYE:
			{
				MS_DEBUG_TAG(rtcp, "ignoring received RTCP BYE");

				break;
			}

			default:
			{
				MS_DEBUG_TAG(
				  rtcp,
				  "unhandled RTCP type received [type:%" PRIu8 "]",
				  static_cast<uint8_t>(packet->GetType()));
			}
		}
	}

	void Transport::SetNewProducerIdFromRequest(Channel::Request* request, std::string& producerId) const
	{
		MS_TRACE();

		auto jsonProducerIdIt = request->internal.find("producerId");

		if (jsonProducerIdIt == request->internal.end() || !jsonProducerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.producerId");

		producerId.assign(jsonProducerIdIt->get<std::string>());

		if (this->mapProducers.find(producerId) != this->mapProducers.end())
			MS_THROW_ERROR("a Producer with same producerId already exists");
	}

	RTC::Producer* Transport::GetProducerFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		auto jsonProducerIdIt = request->internal.find("producerId");

		if (jsonProducerIdIt == request->internal.end() || !jsonProducerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.producerId");

		auto it = this->mapProducers.find(jsonProducerIdIt->get<std::string>());

		if (it == this->mapProducers.end())
			MS_THROW_ERROR("Producer not found");

		RTC::Producer* producer = it->second;

		return producer;
	}

	void Transport::SetNewConsumerIdFromRequest(Channel::Request* request, std::string& consumerId) const
	{
		MS_TRACE();

		auto jsonConsumerIdIt = request->internal.find("consumerId");

		if (jsonConsumerIdIt == request->internal.end() || !jsonConsumerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.consumerId");

		consumerId.assign(jsonConsumerIdIt->get<std::string>());

		if (this->mapConsumers.find(consumerId) != this->mapConsumers.end())
			MS_THROW_ERROR("a Consumer with same consumerId already exists");
	}

	RTC::Consumer* Transport::GetConsumerFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		auto jsonConsumerIdIt = request->internal.find("consumerId");

		if (jsonConsumerIdIt == request->internal.end() || !jsonConsumerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.consumerId");

		auto it = this->mapConsumers.find(jsonConsumerIdIt->get<std::string>());

		if (it == this->mapConsumers.end())
			MS_THROW_ERROR("Consumer not found");

		RTC::Consumer* consumer = it->second;

		return consumer;
	}

	inline RTC::Consumer* Transport::GetConsumerByMediaSsrc(uint32_t ssrc) const
	{
		MS_TRACE();

		auto mapSsrcConsumerIt = this->mapSsrcConsumer.find(ssrc);

		if (mapSsrcConsumerIt == this->mapSsrcConsumer.end())
			return nullptr;

		auto* consumer = mapSsrcConsumerIt->second;

		return consumer;
	}

	void Transport::SendRtcp(uint64_t now)
	{
		MS_TRACE();

		// - Create a CompoundPacket.
		// - Request every Consumer and Producer their RTCP data.
		// - Send the CompoundPacket.

		std::unique_ptr<RTC::RTCP::CompoundPacket> packet(new RTC::RTCP::CompoundPacket());

		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			consumer->GetRtcp(packet.get(), now);

			// Send the RTCP compound packet if there is a sender report.
			if (packet->HasSenderReport())
			{
				packet->Serialize(RTC::RTCP::Buffer);
				SendRtcpCompoundPacket(packet.get());

				// Reset the Compound packet.
				packet.reset(new RTC::RTCP::CompoundPacket());
			}
		}

		for (auto& kv : this->mapProducers)
		{
			auto* producer = kv.second;

			producer->GetRtcp(packet.get(), now);

			// One more RR would exceed the MTU, send the compound packet now.
			if (packet->GetSize() + sizeof(RTCP::ReceiverReport::Header) > RTC::MtuSize)
			{
				packet->Serialize(RTC::RTCP::Buffer);
				SendRtcpCompoundPacket(packet.get());

				// Reset the Compound packet.
				packet.reset(new RTC::RTCP::CompoundPacket());
			}
		}

		if (packet->GetReceiverReportCount() != 0u)
		{
			packet->Serialize(RTC::RTCP::Buffer);
			SendRtcpCompoundPacket(packet.get());
		}
	}

	inline void Transport::OnProducerPaused(RTC::Producer* producer)
	{
		MS_TRACE();

		this->listener->OnTransportProducerPaused(this, producer);
	}

	inline void Transport::OnProducerResumed(RTC::Producer* producer)
	{
		MS_TRACE();

		this->listener->OnTransportProducerResumed(this, producer);
	}

	inline void Transport::OnProducerNewRtpStream(
	  RTC::Producer* producer, RTC::RtpStream* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		this->listener->OnTransportProducerNewRtpStream(this, producer, rtpStream, mappedSsrc);
	}

	inline void Transport::OnProducerRtpStreamScore(
	  RTC::Producer* producer, RTC::RtpStream* rtpStream, uint8_t score)
	{
		MS_TRACE();

		this->listener->OnTransportProducerRtpStreamScore(this, producer, rtpStream, score);
	}

	inline void Transport::OnProducerRtpPacketReceived(RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->listener->OnTransportProducerRtpPacketReceived(this, producer, packet);
	}

	inline void Transport::OnProducerSendRtcpPacket(RTC::Producer* /*producer*/, RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		SendRtcpPacket(packet);
	}

	inline void Transport::OnProducerNeedWorstRemoteFractionLost(
	  RTC::Producer* producer, uint32_t mappedSsrc, uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();

		this->listener->OnTransportNeedWorstRemoteFractionLost(
		  this, producer, mappedSsrc, worstRemoteFractionLost);
	}

	inline void Transport::OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		SendRtpPacket(packet, consumer);
	}

	inline void Transport::OnConsumerRetransmitRtpPacket(RTC::Consumer* consumer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Update http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time.
		{
			uint8_t extenLen;
			uint8_t* extenValue = packet->GetExtension(
			  static_cast<uint8_t>(RTC::RtpHeaderExtensionUri::Type::ABS_SEND_TIME), extenLen);

			if (extenValue && extenLen == 3)
			{
				auto now         = DepLibUV::GetTime();
				auto absSendTime = static_cast<uint32_t>(((now << 18) + 500) / 1000) & 0x00FFFFFF;

				Utils::Byte::Set3Bytes(extenValue, 0, absSendTime);
			}
		}

		SendRtpPacket(packet, consumer);
	}

	inline void Transport::OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc)
	{
		MS_TRACE();

		if (!IsConnected())
		{
			MS_WARN_TAG(rtcp, "ignoring key rame request (transport not connected)");

			return;
		}

		this->listener->OnTransportConsumerKeyFrameRequested(this, consumer, mappedSsrc);
	}

	inline void Transport::onConsumerProducerClosed(RTC::Consumer* consumer)
	{
		MS_TRACE();

		// Remove it from the maps.
		this->mapConsumers.erase(consumer->id);

		for (auto ssrc : consumer->GetMediaSsrcs())
		{
			this->mapSsrcConsumer.erase(ssrc);
		}

		// Notify the listener.
		this->listener->OnTransportConsumerProducerClosed(this, consumer);

		// Delete it.
		delete consumer;
	}

	inline void Transport::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->rtcpTimer)
		{
			auto interval = static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs);
			uint64_t now  = DepLibUV::GetTime();

			SendRtcp(now);

			// Recalculate next RTCP interval.
			if (!this->mapConsumers.empty())
			{
				// Transmission rate in kbps.
				uint32_t rate{ 0 };

				// Get the RTP sending rate.
				for (auto& kv : this->mapConsumers)
				{
					auto* consumer = kv.second;

					rate += consumer->GetTransmissionRate(now) / 1000;
				}

				// Calculate bandwidth: 360 / transmission bandwidth in kbit/s.
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
