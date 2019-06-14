#define MS_CLASS "RTC::Transport"
// #define MS_LOG_DEV

#include "RTC/Transport.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/PipeConsumer.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/SimpleConsumer.hpp"
#include "RTC/SimulcastConsumer.hpp"
#include "RTC/SvcConsumer.hpp"

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

		// Delete all DataProducers.
		for (auto& kv : this->mapDataProducers)
		{
			auto* dataProducer = kv.second;

			delete dataProducer;
		}
		this->mapDataProducers.clear();

		// Delete all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			delete dataConsumer;
		}
		this->mapDataConsumers.clear();

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

		// Delete all DataProducers.
		for (auto& kv : this->mapDataProducers)
		{
			auto* dataProducer = kv.second;

			// Notify the listener.
			this->listener->OnTransportDataProducerClosed(this, dataProducer);

			delete dataProducer;
		}
		this->mapDataProducers.clear();

		// Delete all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			// Notify the listener.
			this->listener->OnTransportDataConsumerClosed(this, dataConsumer);

			delete dataConsumer;
		}
		this->mapDataConsumers.clear();
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

		// Add dataProducerIds.
		jsonObject["dataProducerIds"] = json::array();
		auto jsonDataProducerIdsIt    = jsonObject.find("dataProducerIds");

		for (auto& kv : this->mapDataProducers)
		{
			auto& dataProducerId = kv.first;

			jsonDataProducerIdsIt->emplace_back(dataProducerId);
		}

		// Add dataConsumerIds.
		jsonObject["dataConsumerIds"] = json::array();
		auto jsonDataConsumerIdsIt    = jsonObject.find("dataConsumerIds");

		for (auto& kv : this->mapDataConsumers)
		{
			auto& dataConsumerId = kv.first;

			jsonDataConsumerIdsIt->emplace_back(dataConsumerId);
		}

		// Add rtpListener.
		this->rtpListener.FillJson(jsonObject["rtpListener"]);

		if (this->sctpAssociation)
		{
			// Add sctpParameters.
			this->sctpAssociation->FillJson(jsonObject["sctpParameters"]);

			// Add sctpState.
			switch (this->sctpAssociation->GetState())
			{
				case RTC::SctpAssociation::SctpState::NEW:
					jsonObject["sctpState"] = "new";
					break;
				case RTC::SctpAssociation::SctpState::CONNECTING:
					jsonObject["sctpState"] = "connecting";
					break;
				case RTC::SctpAssociation::SctpState::CONNECTED:
					jsonObject["sctpState"] = "connected";
					break;
				case RTC::SctpAssociation::SctpState::FAILED:
					jsonObject["sctpState"] = "failed";
					break;
				case RTC::SctpAssociation::SctpState::CLOSED:
					jsonObject["sctpState"] = "closed";
					break;
			}

			// Add sctpListener.
			this->sctpListener.FillJson(jsonObject["sctpListener"]);
		}
	}

	void Transport::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::TRANSPORT_DUMP:
			{
				json data = json::object();

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_GET_STATS:
			{
				json data = json::array();

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
					this->rtpListener.RemoveProducer(producer);

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
				json data = json::object();

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
						// This may throw.
						consumer = new RTC::SvcConsumer(consumerId, this, request->data);

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
				json data = json::object();

				data["paused"]         = consumer->IsPaused();
				data["producerPaused"] = consumer->IsProducerPaused();

				consumer->FillJsonScore(data["score"]);

				request->Accept(data);

				// Tell the subclass.
				UserOnNewConsumer(consumer);

				if (IsConnected())
					consumer->TransportConnected();

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_PRODUCE_DATA:
			{
				if (!this->sctpAssociation)
					MS_THROW_ERROR("SCTP not enabled");

				std::string dataProducerId;

				// This may throw.
				SetNewDataProducerIdFromRequest(request, dataProducerId);

				// This may throw.
				auto* dataProducer = new RTC::DataProducer(dataProducerId, this, request->data);

				// Insert the DataProducer into the SctpListener.
				// This may throw. If so, delete the DataProducer and throw.
				try
				{
					this->sctpListener.AddDataProducer(dataProducer);
				}
				catch (const MediaSoupError& error)
				{
					delete dataProducer;

					throw;
				}

				// Notify the listener.
				// This may throw if a DataProducer with same id already exists.
				try
				{
					this->listener->OnTransportNewDataProducer(this, dataProducer);
				}
				catch (const MediaSoupError& error)
				{
					this->sctpListener.RemoveDataProducer(dataProducer);

					delete dataProducer;

					throw;
				}

				// Insert into the map.
				this->mapDataProducers[dataProducerId] = dataProducer;

				MS_DEBUG_DEV("Producer created [dataProducerId:%s]", dataProducerId.c_str());

				json data = json::object();

				dataProducer->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_CONSUME_DATA:
			{
				if (!this->sctpAssociation)
					MS_THROW_ERROR("SCTP not enabled");

				auto jsonDataProducerIdIt = request->internal.find("dataProducerId");

				if (jsonDataProducerIdIt == request->internal.end() || !jsonDataProducerIdIt->is_string())
					MS_THROW_ERROR("request has no internal.dataProducerId");

				std::string dataProducerId = jsonDataProducerIdIt->get<std::string>();
				std::string dataConsumerId;

				// This may throw.
				SetNewDataConsumerIdFromRequest(request, dataConsumerId);

				// This may throw.
				auto* dataConsumer = new RTC::DataConsumer(
				  dataConsumerId, this, request->data, this->sctpAssociation->GetMaxSctpMessageSize());

				// Notify the listener.
				// This may throw if no DataProducer is found.
				try
				{
					this->listener->OnTransportNewDataConsumer(this, dataConsumer, dataProducerId);
				}
				catch (const MediaSoupError& error)
				{
					delete dataConsumer;

					throw;
				}

				// Insert into the maps.
				this->mapDataConsumers[dataConsumerId] = dataConsumer;

				MS_DEBUG_DEV(
				  "DataConsumer created [dataConsumerId:%s, dataProducerId:%s]",
				  dataConsumerId.c_str(),
				  dataProducerId.c_str());

				json data = json::object();

				dataConsumer->FillJson(data);

				request->Accept(data);

				if (IsConnected())
					dataConsumer->TransportConnected();

				if (this->sctpAssociation->GetState() == RTC::SctpAssociation::SctpState::CONNECTED)
					dataConsumer->SctpAssociationConnected();

				// Tell to the SCTP association.
				this->sctpAssociation->HandleDataConsumer(dataConsumer);

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

			case Channel::Request::MethodId::DATA_PRODUCER_CLOSE:
			{
				// This may throw.
				RTC::DataProducer* dataProducer = GetDataProducerFromRequest(request);

				// Remove it from the SctpListener.
				this->sctpListener.RemoveDataProducer(dataProducer);

				// Remove it from the map.
				this->mapDataProducers.erase(dataProducer->id);

				// Notify the listener.
				this->listener->OnTransportDataProducerClosed(this, dataProducer);

				MS_DEBUG_DEV("DataProducer closed [dataProducerId:%s]", dataProducer->id.c_str());

				// Tell the SctpAssociation so it can reset the SCTP stream.
				this->sctpAssociation->DataProducerClosed(dataProducer);

				// Delete it.
				delete dataProducer;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::DATA_CONSUMER_CLOSE:
			{
				// This may throw.
				RTC::DataConsumer* dataConsumer = GetDataConsumerFromRequest(request);

				// Remove it from the maps.
				this->mapDataConsumers.erase(dataConsumer->id);

				// Notify the listener.
				this->listener->OnTransportDataConsumerClosed(this, dataConsumer);

				MS_DEBUG_DEV("DataConsumer closed [dataConsumerId:%s]", dataConsumer->id.c_str());

				// Tell the SctpAssociation so it can reset the SCTP stream.
				this->sctpAssociation->DataConsumerClosed(dataConsumer);

				// Delete it.
				delete dataConsumer;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::DATA_PRODUCER_DUMP:
			case Channel::Request::MethodId::DATA_PRODUCER_GET_STATS:
			{
				// This may throw.
				RTC::DataProducer* dataProducer = GetDataProducerFromRequest(request);

				dataProducer->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::DATA_CONSUMER_DUMP:
			case Channel::Request::MethodId::DATA_CONSUMER_GET_STATS:
			{
				// This may throw.
				RTC::DataConsumer* dataConsumer = GetDataConsumerFromRequest(request);

				dataConsumer->HandleRequest(request);

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->method.c_str());
			}
		}
	}

	void Transport::CreateSctpAssociation(json& data)
	{
		MS_TRACE();

		auto jsonEnableSctpIt = data.find("enableSctp");

		// clang-format off
		if (
			jsonEnableSctpIt != data.end() &&
			jsonEnableSctpIt->is_boolean() &&
			jsonEnableSctpIt->get<bool>()
		)
		// clang-format on
		{
			auto jsonNumSctpStreamsIt     = data.find("numSctpStreams");
			auto jsonMaxSctpMessageSizeIt = data.find("maxSctpMessageSize");
			auto jsonIsDataChannelIt      = data.find("isDataChannel");

			// numSctpStreams is mandatory.
			// clang-format off
			if (
				jsonNumSctpStreamsIt == data.end() ||
				!jsonNumSctpStreamsIt->is_object()
			)
			// clang-format on
			{
				MS_THROW_TYPE_ERROR("wrong numSctpStreams (not an object)");
			}

			auto jsonOSIt  = jsonNumSctpStreamsIt->find("OS");
			auto jsonMISIt = jsonNumSctpStreamsIt->find("MIS");

			// numSctpStreams.OS and numSctpStreams.MIS are mandatory.
			// clang-format off
			if (
				jsonOSIt == jsonNumSctpStreamsIt->end() ||
				!jsonOSIt->is_number_unsigned() ||
				jsonMISIt == jsonNumSctpStreamsIt->end() ||
				!jsonMISIt->is_number_unsigned()
			)
			// clang-format on
			{
				MS_THROW_TYPE_ERROR("wrong numSctpStreams.OS and/or numSctpStreams.MIS (not a number)");
			}

			auto os  = jsonOSIt->get<uint16_t>();
			auto mis = jsonMISIt->get<uint16_t>();

			// maxSctpMessageSize is mandatory.
			// clang-format off
			if (
				jsonMaxSctpMessageSizeIt == data.end() ||
				!jsonMaxSctpMessageSizeIt->is_number_unsigned()
			)
			// clang-format on
			{
				MS_THROW_TYPE_ERROR("wrong maxSctpMessageSize (not a number)");
			}

			auto maxSctpMessageSize = jsonMaxSctpMessageSizeIt->get<size_t>();

			// isDataChannel is optional.
			bool isDataChannel{ false };

			if (jsonIsDataChannelIt != data.end() && jsonIsDataChannelIt->is_boolean())
				isDataChannel = jsonIsDataChannelIt->get<bool>();

			this->sctpAssociation =
			  new RTC::SctpAssociation(this, os, mis, maxSctpMessageSize, isDataChannel);
		}
	}

	void Transport::DestroySctpAssociation()
	{
		MS_TRACE();

		delete this->sctpAssociation;
	}

	void Transport::Connected()
	{
		MS_TRACE();

		// Start the RTCP timer.
		this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));

		// Tell all Consumers.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			consumer->TransportConnected();
		}

		// Tell all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			dataConsumer->TransportConnected();
		}

		// Tell the SctpAssociation.
		if (this->sctpAssociation)
			this->sctpAssociation->Run();
	}

	void Transport::Disconnected()
	{
		MS_TRACE();

		// Stop the RTCP timer.
		this->rtcpTimer->Stop();

		// Tell all Consumers.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			consumer->TransportDisconnected();
		}

		// Tell all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			dataConsumer->TransportDisconnected();
		}
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

						consumer->ReceiveKeyFrameRequest(feedback->GetMessageType(), feedback->GetMediaSsrc());

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

			case RTC::RTCP::Type::XR:
			{
				auto* ser = static_cast<RTC::RTCP::SenderExtendedReportPacket*>(packet);

				for (auto it = ser->Begin(); it != ser->End(); ++it)
				{
					auto& report = (*it);
					if (report->GetSsrc() == 0)
						report->SetSsrc(ser->GetSsrc());

					auto* producer = this->rtpListener.GetProducer(report->GetSsrc());

					if (producer == nullptr)
					{
						MS_WARN_TAG(
						  rtcp,
						  "no Producer found for received Sender Extended Report [ssrc:%" PRIu32 "]",
						  report->GetSsrc());

						continue;
					}

					producer->ReceiveRtcpSenderExtendedReport(report);
				}

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

	void Transport::SetNewDataProducerIdFromRequest(
	  Channel::Request* request, std::string& dataProducerId) const
	{
		MS_TRACE();

		auto jsonDataProducerIdIt = request->internal.find("dataProducerId");

		if (jsonDataProducerIdIt == request->internal.end() || !jsonDataProducerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.dataProducerId");

		dataProducerId.assign(jsonDataProducerIdIt->get<std::string>());

		if (this->mapDataProducers.find(dataProducerId) != this->mapDataProducers.end())
			MS_THROW_ERROR("a DataProducer with same dataProducerId already exists");
	}

	RTC::DataProducer* Transport::GetDataProducerFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		auto jsonDataProducerIdIt = request->internal.find("dataProducerId");

		if (jsonDataProducerIdIt == request->internal.end() || !jsonDataProducerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.dataProducerId");

		auto it = this->mapDataProducers.find(jsonDataProducerIdIt->get<std::string>());

		if (it == this->mapDataProducers.end())
			MS_THROW_ERROR("DataProducer not found");

		RTC::DataProducer* dataProducer = it->second;

		return dataProducer;
	}

	void Transport::SetNewDataConsumerIdFromRequest(
	  Channel::Request* request, std::string& dataConsumerId) const
	{
		MS_TRACE();

		auto jsonDataConsumerIdIt = request->internal.find("dataConsumerId");

		if (jsonDataConsumerIdIt == request->internal.end() || !jsonDataConsumerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.dataConsumerId");

		dataConsumerId.assign(jsonDataConsumerIdIt->get<std::string>());

		if (this->mapDataConsumers.find(dataConsumerId) != this->mapDataConsumers.end())
			MS_THROW_ERROR("a DataConsumer with same dataConsumerId already exists");
	}

	RTC::DataConsumer* Transport::GetDataConsumerFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		auto jsonDataConsumerIdIt = request->internal.find("dataConsumerId");

		if (jsonDataConsumerIdIt == request->internal.end() || !jsonDataConsumerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.dataConsumerId");

		auto it = this->mapDataConsumers.find(jsonDataConsumerIdIt->get<std::string>());

		if (it == this->mapDataConsumers.end())
			MS_THROW_ERROR("DataConsumer not found");

		RTC::DataConsumer* dataConsumer = it->second;

		return dataConsumer;
	}

	void Transport::SendRtcp(uint64_t now)
	{
		MS_TRACE();

		std::unique_ptr<RTC::RTCP::CompoundPacket> packet{ nullptr };

		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			for (auto& rtpStream : consumer->GetRtpStreams())
			{
				// Reset the Compound packet.
				packet.reset(new RTC::RTCP::CompoundPacket());

				consumer->GetRtcp(packet.get(), rtpStream, now);

				// Send the RTCP compound packet if there is a sender report.
				if (packet->HasSenderReport())
				{
					packet->Serialize(RTC::RTCP::Buffer);
					SendRtcpCompoundPacket(packet.get());
				}
			}
		}

		// Reset the Compound packet.
		packet.reset(new RTC::RTCP::CompoundPacket());

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
	  RTC::Producer* producer, RTC::RtpStream* rtpStream, uint8_t score, uint8_t previousScore)
	{
		MS_TRACE();

		this->listener->OnTransportProducerRtpStreamScore(this, producer, rtpStream, score, previousScore);
	}

	inline void Transport::OnProducerRtcpSenderReport(
	  RTC::Producer* producer, RTC::RtpStream* rtpStream, bool first)
	{
		MS_TRACE();

		this->listener->OnTransportProducerRtcpSenderReport(this, producer, rtpStream, first);
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

	inline void Transport::OnConsumerRetransmitRtpPacket(
	  RTC::Consumer* consumer, RTC::RtpPacket* packet, bool probation)
	{
		MS_TRACE();

		// Update http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time if present.
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

		SendRtpPacket(packet, consumer, true, probation);
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

	inline void Transport::OnConsumerProducerClosed(RTC::Consumer* consumer)
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

	inline void Transport::OnDataProducerSctpMessageReceived(
	  RTC::DataProducer* dataProducer, uint32_t ppid, const uint8_t* msg, size_t len)
	{
		MS_TRACE();

		this->listener->OnTransportDataProducerSctpMessageReceived(this, dataProducer, ppid, msg, len);
	}

	inline void Transport::OnDataConsumerSendSctpMessage(
	  RTC::DataConsumer* dataConsumer, uint32_t ppid, const uint8_t* msg, size_t len)
	{
		MS_TRACE();

		this->sctpAssociation->SendSctpMessage(dataConsumer, ppid, msg, len);
	}

	inline void Transport::OnDataConsumerDataProducerClosed(RTC::DataConsumer* dataConsumer)
	{
		MS_TRACE();

		// Remove it from the maps.
		this->mapDataConsumers.erase(dataConsumer->id);

		// Notify the listener.
		this->listener->OnTransportDataConsumerDataProducerClosed(this, dataConsumer);

		// Tell the SctpAssociation so it can reset the SCTP stream.
		this->sctpAssociation->DataConsumerClosed(dataConsumer);

		// Delete it.
		delete dataConsumer;
	}

	inline void Transport::OnSctpAssociationConnecting(RTC::SctpAssociation* /*sctpAssociation*/)
	{
		MS_TRACE();

		// Notify the Node Transport.
		json data = json::object();

		data["sctpState"] = "connecting";

		Channel::Notifier::Emit(this->id, "sctpstatechange", data);
	}

	inline void Transport::OnSctpAssociationConnected(RTC::SctpAssociation* /*sctpAssociation*/)
	{
		MS_TRACE();

		// Tell all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			dataConsumer->SctpAssociationConnected();
		}

		// Notify the Node Transport.
		json data = json::object();

		data["sctpState"] = "connected";

		Channel::Notifier::Emit(this->id, "sctpstatechange", data);
	}

	inline void Transport::OnSctpAssociationFailed(RTC::SctpAssociation* /*sctpAssociation*/)
	{
		MS_TRACE();

		// Tell all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			dataConsumer->SctpAssociationClosed();
		}

		// Notify the Node Transport.
		json data = json::object();

		data["sctpState"] = "failed";

		Channel::Notifier::Emit(this->id, "sctpstatechange", data);
	}

	inline void Transport::OnSctpAssociationClosed(RTC::SctpAssociation* /*sctpAssociation*/)
	{
		MS_TRACE();

		// Tell all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			dataConsumer->SctpAssociationClosed();
		}

		// Notify the Node Transport.
		json data = json::object();

		data["sctpState"] = "closed";

		Channel::Notifier::Emit(this->id, "sctpstatechange", data);
	}

	inline void Transport::OnSctpAssociationSendData(
	  RTC::SctpAssociation* /*sctpAssociation*/, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Pass it to the subclass.
		UserOnSendSctpData(data, len);
	}

	inline void Transport::OnSctpAssociationMessageReceived(
	  RTC::SctpAssociation* /*sctpAssociation*/,
	  uint16_t streamId,
	  uint32_t ppid,
	  const uint8_t* msg,
	  size_t len)
	{
		MS_TRACE();

		RTC::DataProducer* dataProducer = this->sctpListener.GetDataProducer(streamId);

		if (dataProducer == nullptr)
		{
			MS_WARN_TAG(
			  sctp, "no suitable DataProducer for received SCTP message [streamId:%" PRIu16 "]", streamId);

			return;
		}

		// Pass the SCTP message to the corresponding DataProducer.
		dataProducer->ReceiveSctpMessage(ppid, msg, len);
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
