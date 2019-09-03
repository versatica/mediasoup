#define MS_CLASS "RTC::Transport"
#define MS_LOG_DEV

#include "RTC/Transport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/PipeConsumer.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/SimpleConsumer.hpp"
#include "RTC/SimulcastConsumer.hpp"
#include "RTC/SvcConsumer.hpp"
#include <iterator> // std::ostream_iterator
#include <map>      // std::multimap
#include <sstream>  // std::ostringstream

namespace RTC
{
	/* Static. */

	static constexpr uint64_t IncomingBitrateLimitationByRembInterval{ 1500 }; // In ms.

	/* Instance methods. */

	Transport::Transport(const std::string& id, Listener* listener, json& data)
	  : id(id), listener(listener)
	{
		MS_TRACE();

		auto jsonInitialAvailableOutgoingBitrateIt = data.find("initialAvailableOutgoingBitrate");

		if (jsonInitialAvailableOutgoingBitrateIt != data.end())
		{
			if (!jsonInitialAvailableOutgoingBitrateIt->is_number_unsigned())
				MS_THROW_TYPE_ERROR("wrong initialAvailableOutgoingBitrate (not a number)");

			this->initialAvailableOutgoingBitrate = jsonInitialAvailableOutgoingBitrateIt->get<uint32_t>();
		}

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

		// Create the RTCP timer.
		this->rtcpTimer = new Timer(this);
	}

	Transport::~Transport()
	{
		MS_TRACE();

		// Set the destroying flag.
		this->destroying = true;

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

		// Delete SCTP association.
		delete this->sctpAssociation;
		this->sctpAssociation = nullptr;

		// Delete the RTCP timer.
		delete this->rtcpTimer;
		this->rtcpTimer = nullptr;

		// Delete REMB client.
		delete this->rembClient;
		this->rembClient = nullptr;

		// Delete REMB server.
		delete this->rembServer;
		this->rembServer = nullptr;

		// Delete Transport-CC client.
		delete this->tccClient;
		this->tccClient = nullptr;

		// Delete Transport-CC server.
		delete this->tccServer;
		this->tccServer = nullptr;
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

		// Add headerExtensionIds.
		jsonObject["rtpHeaderExtensions"] = json::object();
		auto jsonRtpHeaderExtensionsIt    = jsonObject.find("rtpHeaderExtensions");

		if (this->rtpHeaderExtensionIds.mid != 0u)
			(*jsonRtpHeaderExtensionsIt)["mid"] = this->rtpHeaderExtensionIds.mid;

		if (this->rtpHeaderExtensionIds.rid != 0u)
			(*jsonRtpHeaderExtensionsIt)["rid"] = this->rtpHeaderExtensionIds.rid;

		if (this->rtpHeaderExtensionIds.rrid != 0u)
			(*jsonRtpHeaderExtensionsIt)["rrid"] = this->rtpHeaderExtensionIds.rrid;

		if (this->rtpHeaderExtensionIds.absSendTime != 0u)
			(*jsonRtpHeaderExtensionsIt)["absSendTime"] = this->rtpHeaderExtensionIds.absSendTime;

		if (this->rtpHeaderExtensionIds.transportWideCc01 != 0u)
			(*jsonRtpHeaderExtensionsIt)["transportWideCc01"] =
			  this->rtpHeaderExtensionIds.transportWideCc01;

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

	void Transport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		jsonArray.emplace_back(json::value_t::object);
		auto& jsonObject = jsonArray[0];

		// Add transportId.
		jsonObject["transportId"] = this->id;

		// Add timestamp.
		jsonObject["timestamp"] = DepLibUV::GetTime();

		if (this->sctpAssociation)
		{
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
		}

		// Add bytesReceived.
		jsonObject["bytesReceived"] = this->recvTransmission.GetBytes();

		// Add bytesSent.
		jsonObject["bytesSent"] = this->sendTransmission.GetBytes();

		// Add recvBitrate.
		jsonObject["recvBitrate"] = this->recvTransmission.GetRate(DepLibUV::GetTime());

		// Add sendBitrate.
		jsonObject["sendBitrate"] = this->sendTransmission.GetRate(DepLibUV::GetTime());

		// Add availableOutgoingBitrate.
		if (this->rembClient)
			jsonObject["availableOutgoingBitrate"] = this->rembClient->GetAvailableBitrate();
		else if (this->tccClient)
			jsonObject["availableOutgoingBitrate"] = this->tccClient->GetAvailableBitrate();

		// Add availableIncomingBitrate.
		if (this->rembServer)
			jsonObject["availableIncomingBitrate"] = this->rembServer->GetAvailableBitrate();

		// Add maxIncomingBitrate.
		if (this->maxIncomingBitrate != 0u)
			jsonObject["maxIncomingBitrate"] = this->maxIncomingBitrate;
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

			case Channel::Request::MethodId::TRANSPORT_SET_MAX_INCOMING_BITRATE:
			{
				static constexpr uint32_t MinBitrate{ 100000 };

				auto jsonBitrateIt = request->data.find("bitrate");

				if (jsonBitrateIt == request->data.end() || !jsonBitrateIt->is_number_unsigned())
					MS_THROW_TYPE_ERROR("missing bitrate");

				this->maxIncomingBitrate = jsonBitrateIt->get<uint32_t>();

				if (this->maxIncomingBitrate != 0u && this->maxIncomingBitrate < MinBitrate)
					this->maxIncomingBitrate = MinBitrate;

				MS_DEBUG_TAG(bwe, "maximum incoming bitrate set to %" PRIu32, this->maxIncomingBitrate);

				request->Accept();

				// May set REMB based incoming bitrate limitation.
				MaySetIncomingBitrateLimitationByRemb();

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

				if (producerRtpHeaderExtensionIds.transportWideCc01 != 0u)
					this->rtpHeaderExtensionIds.transportWideCc01 =
					  producerRtpHeaderExtensionIds.transportWideCc01;

				// Create status response.
				json data = json::object();

				data["type"] = RTC::RtpParameters::GetTypeString(producer->GetType());

				request->Accept(data);

				// Check if TransportCongestionControl server or REMB server must be
				// created.
				auto& rtpHeaderExtensionIds = producer->GetRtpHeaderExtensionIds();
				auto& codecs                = producer->GetRtpParameters().codecs;

				// Set TransportCongestionControl server:
				// - if not already set, and
				// - if there it not REMB server, and
				// - there is transport-wide-cc-01 RTP header extension, and
				// - there is "transport-cc" in codecs RTCP feedback.
				//
				// clang-format off
				if (
					!this->tccServer &&
					!this->rembServer &&
					rtpHeaderExtensionIds.transportWideCc01 != 0u &&
					std::any_of(
						codecs.begin(), codecs.end(), [](const RTC::RtpCodecParameters& codec)
						{
							return std::any_of(
								codec.rtcpFeedback.begin(), codec.rtcpFeedback.end(), [](const RTC::RtcpFeedback& fb)
								{
									return fb.type == "transport-cc";
								});
						})
				)
				// clang-format on
				{
					MS_DEBUG_TAG(bwe, "enabling TransportCongestionControl server");

					this->tccServer = new RTC::TransportCongestionControlServer(this, RTC::MtuSize);

					// If the transport is connected, tell the Transport-CC server.
					if (IsConnected())
						this->tccServer->TransportConnected();
				}

				// Set REMB server:
				// - if not already set, and
				// - if there it not TransportCongestionControl server, and
				// - there is abs-send-time RTP header extension, and
				// - there is "remb" in codecs RTCP feedback.
				//
				// clang-format off
				if (
					!this->rembServer &&
					!this->tccServer &&
					rtpHeaderExtensionIds.absSendTime != 0u &&
					std::any_of(
						codecs.begin(), codecs.end(), [](const RTC::RtpCodecParameters& codec)
						{
							return std::any_of(
								codec.rtcpFeedback.begin(), codec.rtcpFeedback.end(), [](const RTC::RtcpFeedback& fb)
								{
									return fb.type == "goog-remb";
								});
						})
				)
				// clang-format on
				{
					MS_DEBUG_TAG(bwe, "enabling REMB server");

					this->rembServer = new RTC::RembServer::RemoteBitrateEstimatorAbsSendTime(this);
				}

				// May set REMB based incoming bitrate limitation.
				MaySetIncomingBitrateLimitationByRemb();

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

				// Check if Transport Congestion Control client must be created.
				auto& rtpHeaderExtensionIds = consumer->GetRtpHeaderExtensionIds();
				auto& codecs                = consumer->GetRtpParameters().codecs;

				// Set TCC client bitrate estimator:
				// - if not already set, and
				// - REMB client is not set, and
				// - Consumer is simulcast or SVC, and
				// - there is transport-wide-cc-01 RTP header extension, and
				// - there is "transport-cc" in codecs RTCP feedback.
				//
				// clang-format off
				if (
					!this->tccClient &&
					!this->rembClient &&
					(
						consumer->GetType() == RTC::RtpParameters::Type::SIMULCAST ||
						consumer->GetType() == RTC::RtpParameters::Type::SVC
					) &&
					rtpHeaderExtensionIds.transportWideCc01 != 0u &&
					std::any_of(
						codecs.begin(), codecs.end(), [](const RTC::RtpCodecParameters& codec)
						{
							return std::any_of(
								codec.rtcpFeedback.begin(), codec.rtcpFeedback.end(), [](const RTC::RtcpFeedback& fb)
								{
									return fb.type == "transport-cc";
								});
						})
				)
				// clang-format on
				{
					MS_DEBUG_TAG(bwe, "enabling TCC client");

					// Tell all the Consumers that we are gonna manage their bitrate.
					for (auto& kv : this->mapConsumers)
					{
						auto* consumer = kv.second;

						consumer->SetExternallyManagedBitrate();
					};

					// TODO: When unified with REMB we must check properly set bweType.
					RTC::TransportCongestionControlClient::BweType bweType;

					bweType = RTC::TransportCongestionControlClient::BweType::TRANSPORT_WIDE_CONGESTION;

					this->tccClient = new RTC::TransportCongestionControlClient(
					  this, bweType, this->initialAvailableOutgoingBitrate);

					// If the transport is connected, tell the Transport-CC client.
					if (IsConnected())
						this->tccClient->TransportConnected();
				}

				// Set REMB client bitrate estimator:
				// - if not already set, and
				// - TCC client is not set, and
				// - Consumer is simulcast or SVC, and
				// - there is abs-send-time RTP header extension, and
				// - there is "remb" in codecs RTCP feedback.
				//
				// clang-format off
				if (
					!this->rembClient &&
					!this->tccClient &&
					(
						consumer->GetType() == RTC::RtpParameters::Type::SIMULCAST ||
						consumer->GetType() == RTC::RtpParameters::Type::SVC
					) &&
					rtpHeaderExtensionIds.absSendTime != 0u &&
					std::any_of(
						codecs.begin(), codecs.end(), [](const RTC::RtpCodecParameters& codec)
						{
							return std::any_of(
								codec.rtcpFeedback.begin(), codec.rtcpFeedback.end(), [](const RTC::RtcpFeedback& fb)
								{
									return fb.type == "goog-remb";
								});
						})
				)
				// clang-format on
				{
					MS_DEBUG_TAG(bwe, "enabling REMB client");

					// Tell all the Consumers that we are gonna manage their bitrate.
					for (auto& kv : this->mapConsumers)
					{
						auto* consumer = kv.second;

						consumer->SetExternallyManagedBitrate();
					};

					this->rembClient = new RTC::RembClient(this, this->initialAvailableOutgoingBitrate);
				}

				// If applicable, tell the new Consumer that we are gonna manage its
				// bitrate.
				if (this->rembClient || this->tccClient)
					consumer->SetExternallyManagedBitrate();

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

				MS_DEBUG_DEV("DataProducer created [dataProducerId:%s]", dataProducerId.c_str());

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

	void Transport::Connected()
	{
		MS_TRACE();

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
			this->sctpAssociation->TransportConnected();

		// Start the RTCP timer.
		this->rtcpTimer->Start(static_cast<uint64_t>(RTC::RTCP::MaxVideoIntervalMs / 2));

		// Tell the Transport-CC client.
		if (this->tccClient)
			this->tccClient->TransportConnected();

		// Tell the Transport-CC server.
		if (this->tccServer)
			this->tccServer->TransportConnected();
	}

	void Transport::Disconnected()
	{
		MS_TRACE();

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

		// Stop the RTCP timer.
		this->rtcpTimer->Stop();

		// Tell the Transport-CC client.
		if (this->tccClient)
			this->tccClient->TransportDisconnected();

		// Tell the Transport-CC server.
		if (this->tccServer)
			this->tccServer->TransportDisconnected();
	}

	void Transport::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Apply the Transport RTP header extension ids so the RTP listener can use them.
		packet->SetMidExtensionId(this->rtpHeaderExtensionIds.mid);
		packet->SetRidExtensionId(this->rtpHeaderExtensionIds.rid);
		packet->SetRepairedRidExtensionId(this->rtpHeaderExtensionIds.rrid);
		packet->SetAbsSendTimeExtensionId(this->rtpHeaderExtensionIds.absSendTime);
		packet->SetTransportWideCc01ExtensionId(this->rtpHeaderExtensionIds.transportWideCc01);

		auto now = DepLibUV::GetTime();

		// Feed the REMB server.
		if (this->rembServer)
		{
			uint32_t absSendTime;

			if (packet->ReadAbsSendTime(absSendTime))
			{
				this->rembServer->IncomingPacket(now, packet->GetPayloadLength(), *packet, absSendTime);
			}
		}

		// Feed the TransportCongestionControl server.
		if (this->tccServer)
		{
			uint16_t wideSeqNumber;

			if (packet->ReadTransportWideCc01(wideSeqNumber))
			{
				// Update the RTCP media SSRC of the ongoing Transport Feedback packet.
				this->tccServer->SetRtcpSsrcs(0, packet->GetSsrc());

				this->tccServer->IncomingPacket(now, wideSeqNumber);
			}
		}

		// Get the associated Producer.
		RTC::Producer* producer = this->rtpListener.GetProducer(packet);

		if (!producer)
		{
			MS_WARN_TAG(
			  rtp,
			  "no suitable Producer for received RTP packet [ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]",
			  packet->GetSsrc(),
			  packet->GetPayloadType());

			delete packet;

			return;
		}

		// MS_DEBUG_DEV(
		//   "RTP packet received [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", producerId:%s]",
		//   packet->GetSsrc(),
		//   packet->GetPayloadType(),
		//   producer->id.c_str());

		// Pass the RTP packet to the corresponding Producer.
		producer->ReceiveRtpPacket(packet);

		delete packet;

		// If REMB based incoming bitrate limitation is enabled, check whether
		// we should send a REMB now.
		// clang-format off
		if (
			this->incomingBitrateLimitedByRemb &&
			now - this->lastRembSentAt > IncomingBitrateLimitationByRembInterval
		)
		// clang-format on
		{
			RTC::RTCP::FeedbackPsRembPacket packet(0u, 0u);

			packet.SetBitrate(this->maxIncomingBitrate);
			packet.Serialize(RTC::RTCP::Buffer);

			SendRtcpPacket(&packet);

			this->lastRembSentAt = now;
		}
	}

	void Transport::ReceiveRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		// Handle each RTCP packet.
		while (packet)
		{
			HandleRtcpPacket(packet);

			auto* previousPacket = packet;

			packet = packet->GetNext();

			delete previousPacket;
		}
	}

	void Transport::ReceiveSctpData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!this->sctpAssociation)
		{
			MS_DEBUG_TAG(sctp, "ignoring SCTP packet (SCTP not enabled)");

			return;
		}

		// Pass it to the SctpAssociation.
		this->sctpAssociation->ProcessSctpData(data, len);
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

	void Transport::HandleRtcpPacket(RTC::RTCP::Packet* packet)
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

					if (!consumer)
					{
						// Special case for the RTP probator.
						if (report->GetSsrc() == RTC::RtpProbationSsrc)
						{
							// TODO: We should pass the RR to the tccClient (and just RR for the
							// probation stream).

							// TODO: Convert report to ReportBlock and pass to tccClient.
							// RTCPReportBlock in include/RTC/SendTransportController/rtp_rtcp_defines.h
							//
							// NOTE: consumer->GetRtt() is already implemented.
							//
							// if (this->tccClient)
							// {
							// this->tccClient->ReceiveRtcpReceiverReport(report, consumer->GetRtt(),
							// DepLibUV::GetTime());
							// }

							continue;
						}

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

						if (!consumer)
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
							// TMP: ignore REMB.
							/*
							auto* remb = static_cast<RTC::RTCP::FeedbackPsRembPacket*>(afb);

							// Pass it to the REMB client.
							if (this->rembClient)
							  this->rembClient->ReceiveRembFeedback(remb);

							// Pass it to the TCC client.
							if (this->tccClient)
							  this->tccClient->ReceiveEstimatedBitrate(remb->GetBitrate());
							*/

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

				// If no Consumer is found and this is not a Transport Feedback for the
				// probation SSRC, ignore it.
				//
				// clang-format off
				if (
					!consumer &&
					(
						feedback->GetMediaSsrc() != RTC::RtpProbationSsrc ||
						feedback->GetMessageType() != RTC::RTCP::FeedbackRtp::MessageType::TCC
					)
				)
				// clang-format on
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

					case RTC::RTCP::FeedbackRtp::MessageType::TCC:
					{
						auto* feedback = static_cast<RTC::RTCP::FeedbackRtpTransportPacket*>(packet);

						// TODO: REMOVE
						// feedback->Dump();
						// MS_DUMP_DATA(feedback->GetData(), feedback->GetSize());

						if (this->tccClient)
							this->tccClient->ReceiveRtcpTransportFeedback(feedback);

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

				// Even if Sender Report packet can only contains one report.
				for (auto it = sr->Begin(); it != sr->End(); ++it)
				{
					auto& report = (*it);
					// Get the producer associated to the SSRC indicated in the report.
					auto* producer = this->rtpListener.GetProducer(report->GetSsrc());

					if (!producer)
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

					if (!producer)
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
				auto* xr = static_cast<RTC::RTCP::ExtendedReportPacket*>(packet);

				for (auto it = xr->Begin(); it != xr->End(); ++it)
				{
					auto& report = (*it);

					switch (report->GetType())
					{
						case RTC::RTCP::ExtendedReportBlock::Type::DLRR:
						{
							auto* dlrr = static_cast<RTC::RTCP::DelaySinceLastRr*>(report);

							for (auto it2 = dlrr->Begin(); it2 != dlrr->End(); ++it2)
							{
								auto& ssrcInfo = (*it2);

								// SSRC should be filled in the sub-block.
								if (ssrcInfo->GetSsrc() == 0)
									ssrcInfo->SetSsrc(xr->GetSsrc());

								auto* producer = this->rtpListener.GetProducer(ssrcInfo->GetSsrc());

								if (!producer)
								{
									MS_WARN_TAG(
									  rtcp,
									  "no Producer found for received Sender Extended Report [ssrc:%" PRIu32 "]",
									  ssrcInfo->GetSsrc());

									continue;
								}

								producer->ReceiveRtcpXrDelaySinceLastRr(ssrcInfo);
							}

							break;
						}

						default:;
					}
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

	void Transport::SendRtcp(uint64_t now)
	{
		MS_TRACE();

		std::unique_ptr<RTC::RTCP::CompoundPacket> packet{ nullptr };

		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			for (auto* rtpStream : consumer->GetRtpStreams())
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

	void Transport::DistributeAvailableOutgoingBitrate()
	{
		MS_TRACE();

		MS_ASSERT(this->rembClient || this->tccClient, "no REMB client nor Transport-CC client");

		std::multimap<uint16_t, RTC::Consumer*> multimapPriorityConsumer;
		uint16_t totalPriorities{ 0 };

		// Fill the map with Consumers and their priority (if > 0).
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;
			auto priority  = consumer->GetBitratePriority();

			if (priority > 0)
			{
				multimapPriorityConsumer.emplace(priority, consumer);
				totalPriorities += priority;
			}
		}

		// Nobody wants bitrate. Exit.
		if (totalPriorities == 0)
			return;

		uint32_t availableBitrate;

		if (this->rembClient)
		{
			availableBitrate = this->rembClient->GetAvailableBitrate();

			this->rembClient->RescheduleNextAvailableBitrateEvent();
		}
		else
		{
			availableBitrate = this->tccClient->GetAvailableBitrate();

			// TODO: Not necessary when creating tccClient with initial bitrate.
			// TODO: ibc: no, this must not happen. Let's see how to fix it.
			if (availableBitrate == 0)
				availableBitrate = this->initialAvailableOutgoingBitrate;

			// TODO: Implement it.
			// this->tccClient->RescheduleNextAvailableBitrateEvent();
		}

		MS_DEBUG_DEV("before iterations [availableBitrate:%" PRIu32 "]", availableBitrate);

		uint32_t remainingBitrate = availableBitrate;

		// First of all, redistribute the available bitrate by taking into account
		// Consumers' priorities.
		for (auto it = multimapPriorityConsumer.rbegin(); it != multimapPriorityConsumer.rend(); ++it)
		{
			auto priority    = it->first;
			auto* consumer   = it->second;
			uint32_t bitrate = (availableBitrate * priority) / totalPriorities;

			MS_DEBUG_DEV(
			  "main bitrate for Consumer [priority:%" PRIu16 ", bitrate:%" PRIu32 ", consumerId:%s]",
			  priority,
			  bitrate,
			  consumer->id.c_str());

			uint32_t usedBitrate;

			if (this->rembClient)
				usedBitrate = consumer->UseAvailableBitrate(bitrate, /*considerLoss*/ true);
			else if (this->tccClient)
				usedBitrate = consumer->UseAvailableBitrate(bitrate, /*considerLoss*/ false);
			else
				MS_ABORT("neither REMB client nor TCC client is set");

			if (usedBitrate <= remainingBitrate)
				remainingBitrate -= usedBitrate;
			else
				remainingBitrate = 0;
		}

		MS_DEBUG_DEV("after first main iteration [remainingBitrate:%" PRIu32 "]", remainingBitrate);

		// Then redistribute the remaining bitrate by allowing Consumers to increase
		// layer by layer.
		while (remainingBitrate >= 2000)
		{
			auto previousRemainingBitrate = remainingBitrate;

			for (auto it = multimapPriorityConsumer.rbegin(); it != multimapPriorityConsumer.rend(); ++it)
			{
				auto* consumer = it->second;

				MS_DEBUG_DEV(
				  "layer bitrate for Consumer [bitrate:%" PRIu32 ", consumerId:%s]",
				  remainingBitrate,
				  consumer->id.c_str());

				uint32_t usedBitrate;

				if (this->rembClient)
					usedBitrate = consumer->IncreaseTemporalLayer(remainingBitrate, /*considerLoss*/ true);
				else if (this->tccClient)
					usedBitrate = consumer->IncreaseTemporalLayer(remainingBitrate, /*considerLoss*/ false);
				else
					MS_ABORT("neither REMB client nor TCC client is set");

				MS_ASSERT(usedBitrate <= remainingBitrate, "Consumer used more layer bitrate than given");

				remainingBitrate -= usedBitrate;

				// No more.
				if (remainingBitrate < 2000)
					break;
			}

			// If no Consumer used bitrate, exit the loop.
			if (remainingBitrate == previousRemainingBitrate)
				break;
		}

		MS_DEBUG_DEV("after layer-by-layer iteration [remainingBitrate:%" PRIu32 "]", remainingBitrate);

		// Finally instruct Consumers to apply their computed layers.
		for (auto it = multimapPriorityConsumer.rbegin(); it != multimapPriorityConsumer.rend(); ++it)
		{
			auto* consumer = it->second;

			consumer->ApplyLayers();
		}
	}

	void Transport::ComputeOutgoingDesiredBitrate()
	{
		MS_TRACE();

		MS_ASSERT(this->rembClient || this->tccClient, "no REMB client nor Transport-CC client");

		uint32_t totalDesiredBitrate{ 0u };

		for (auto& kv : this->mapConsumers)
		{
			auto* consumer      = kv.second;
			auto desiredBitrate = consumer->GetDesiredBitrate();

			totalDesiredBitrate += desiredBitrate;
		}

		MS_DEBUG_DEV("total desired bitrate: %" PRIu32, totalDesiredBitrate);

		// TODO:
		// Must adjust these values.
		this->tccClient->SetDesiredBitrates(
		  totalDesiredBitrate / 2, totalDesiredBitrate / 4, totalDesiredBitrate);
	}

	void Transport::MaySetIncomingBitrateLimitationByRemb()
	{
		MS_TRACE();

		// Enable REMB based incoming bitrate limitation if:
		// - REMB server is not running (the endpoint may be using Transport-CC instead), and
		// - abs-send-time RTP extension has been negotiated, and
		// - the app called setMaxIncomingBitrate() with a value > 0.
		//
		// clang-format off
		if (
			!this->rembServer &&
			this->rtpHeaderExtensionIds.absSendTime != 0u &&
			this->maxIncomingBitrate > 0u
		)
		// clang-format on
		{
			// If it was already set, do nothing.
			if (this->incomingBitrateLimitedByRemb)
				return;

			MS_DEBUG_TAG(
			  bwe,
			  "enabling REMB based incoming bitrate limitation [bitrate:%" PRIu32 "]",
			  this->maxIncomingBitrate);

			this->incomingBitrateLimitedByRemb = true;
		}
		else
		{
			// If it was already unset, do nothing.
			if (!this->incomingBitrateLimitedByRemb)
				return;

			MS_DEBUG_TAG(bwe, "disabling REMB based incoming bitrate limitation");

			this->incomingBitrateLimitedByRemb = false;

			// NOTE: By sending a REMB with bitrate set to 0, libwebrtc resets the
			// latest received REMB value.
			RTC::RTCP::FeedbackPsRembPacket packet(0u, 0u);

			packet.SetBitrate(0);
			packet.Serialize(RTC::RTCP::Buffer);

			// NOTE: Send two packets just in case the first one is lost.
			SendRtcpPacket(&packet);
			SendRtcpPacket(&packet);
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

	inline void Transport::OnConsumerSendRtpPacket(RTC::Consumer* /*consumer*/, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Update transport wide sequence number if present.
		if (packet->UpdateTransportWideCc01(this->transportWideSeq + 1))
		{
			this->transportWideSeq++;

			if (this->tccClient)
			{
				// Indicate the pacer (and prober) that a packet is to be sent.
				this->tccClient->InsertPacket(packet->GetSize());

				auto* tccClient = this->tccClient;
				webrtc::RtpPacketSendInfo packetInfo;

				packetInfo.ssrc                      = packet->GetSsrc();
				packetInfo.transport_sequence_number = this->transportWideSeq;
				packetInfo.has_rtp_sequence_number   = true;
				packetInfo.rtp_sequence_number       = packet->GetSequenceNumber();
				packetInfo.length                    = packet->GetSize();
				packetInfo.pacing_info               = this->tccClient->GetPacingInfo();

				SendRtpPacket(packet, [&packetInfo, tccClient](bool sent) {
					if (sent)
						tccClient->PacketSent(packetInfo, DepLibUV::GetTime());
				});

				return;
			}
		}

		SendRtpPacket(packet);
	}

	inline void Transport::OnConsumerRetransmitRtpPacket(RTC::Consumer* /*consumer*/, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Update abs-send-time if present.
		packet->UpdateAbsSendTime(DepLibUV::GetTime());

		// Update transport wide sequence number if present.
		if (packet->UpdateTransportWideCc01(this->transportWideSeq + 1))
		{
			this->transportWideSeq++;

			if (this->tccClient)
			{
				// Indicate the pacer (and prober) that a packet is to be sent.
				this->tccClient->InsertPacket(packet->GetSize());

				auto* tccClient = this->tccClient;
				webrtc::RtpPacketSendInfo packetInfo;

				packetInfo.ssrc                      = packet->GetSsrc();
				packetInfo.transport_sequence_number = this->transportWideSeq;
				packetInfo.has_rtp_sequence_number   = true;
				packetInfo.rtp_sequence_number       = packet->GetSequenceNumber();
				packetInfo.length                    = packet->GetSize();
				packetInfo.pacing_info               = this->tccClient->GetPacingInfo();

				SendRtpPacket(packet, [&packetInfo, tccClient](bool sent) {
					if (sent)
						tccClient->PacketSent(packetInfo, DepLibUV::GetTime());
				});

				return;
			}
		}

		SendRtpPacket(packet);
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

	inline void Transport::OnConsumerNeedBitrateChange(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();

		DistributeAvailableOutgoingBitrate();
		ComputeOutgoingDesiredBitrate();
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

		// Ignore if destroying.
		// NOTE: This is because when the child class (i.e. WebRtcTransport) is deleted,
		// its destructor is called first and then the parent Transport's destructor,
		// and we would end here calling SendSctpData() which is an abstract method.
		if (this->destroying)
			return;

		if (this->sctpAssociation)
			SendSctpData(data, len);
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

		if (!dataProducer)
		{
			MS_WARN_TAG(
			  sctp, "no suitable DataProducer for received SCTP message [streamId:%" PRIu16 "]", streamId);

			return;
		}

		// Pass the SCTP message to the corresponding DataProducer.
		dataProducer->ReceiveSctpMessage(ppid, msg, len);
	}

	inline void Transport::OnRembClientAvailableBitrate(
	  RTC::RembClient* /*rembClient*/, uint32_t availableBitrate, uint32_t previousAvailableBitrate)
	{
		MS_TRACE();

		MS_DEBUG_DEV(
		  "outgoing available bitrate [now:%" PRIu32 ", before:%" PRIu32 "]",
		  availableBitrate,
		  previousAvailableBitrate);

		DistributeAvailableOutgoingBitrate();
		ComputeOutgoingDesiredBitrate();
	}

	inline void Transport::OnRembServerAvailableBitrate(
	  const RTC::RembServer::RemoteBitrateEstimator* /*rembServer*/,
	  const std::vector<uint32_t>& ssrcs,
	  uint32_t availableBitrate)
	{
		MS_TRACE();

		// Limit announced bitrate if requested via API.
		if (this->maxIncomingBitrate != 0u)
			availableBitrate = std::min(availableBitrate, this->maxIncomingBitrate);

#ifdef MS_LOG_DEV
		std::ostringstream ssrcsStream;

		if (!ssrcs.empty())
		{
			std::copy(ssrcs.begin(), ssrcs.end() - 1, std::ostream_iterator<uint32_t>(ssrcsStream, ","));
			ssrcsStream << ssrcs.back();
		}

		MS_DEBUG_DEV(
		  "sending RTCP REMB packet [bitrate:%" PRIu32 ", ssrcs:%s]",
		  availableBitrate,
		  ssrcsStream.str().c_str());
#endif

		RTC::RTCP::FeedbackPsRembPacket packet(0u, 0u);

		packet.SetBitrate(availableBitrate);
		packet.SetSsrcs(ssrcs);
		packet.Serialize(RTC::RTCP::Buffer);

		SendRtcpPacket(&packet);
	}

	inline void Transport::OnTransportCongestionControlClientAvailableBitrate(
	  RTC::TransportCongestionControlClient* /*tccClient*/,
	  uint32_t availableBitrate,
	  uint32_t previousAvailableBitrate)
	{
		MS_TRACE();

		MS_DEBUG_DEV(
		  "outgoing available bitrate [now:%" PRIu32 ", before:%" PRIu32 "]",
		  availableBitrate,
		  previousAvailableBitrate);

		DistributeAvailableOutgoingBitrate();
		ComputeOutgoingDesiredBitrate();
	}

	inline void Transport::OnTransportCongestionControlClientSendRtpPacket(
	  RTC::TransportCongestionControlClient* tccClient,
	  RTC::RtpPacket* packet,
	  const webrtc::PacedPacketInfo& pacingInfo)
	{
		MS_TRACE();

		// Update transport wide sequence number if present.
		if (packet->UpdateTransportWideCc01(this->transportWideSeq + 1))
		{
			this->transportWideSeq++;

			// Indicate the pacer (and prober) that a packet is to be sent.
			this->tccClient->InsertPacket(packet->GetSize());

			webrtc::RtpPacketSendInfo packetInfo;

			packetInfo.ssrc                      = packet->GetSsrc();
			packetInfo.transport_sequence_number = this->transportWideSeq;
			packetInfo.has_rtp_sequence_number   = true;
			packetInfo.rtp_sequence_number       = packet->GetSequenceNumber();
			packetInfo.length                    = packet->GetSize();
			packetInfo.pacing_info               = pacingInfo;

			SendRtpPacket(packet, [&packetInfo, tccClient](bool sent) {
				if (sent)
					tccClient->PacketSent(packetInfo, DepLibUV::GetTime());
			});
		}
		else
		{
			MS_ERROR("packet->UpdateTransportWideCc01() returned false");
		}
	}

	inline void Transport::OnTransportCongestionControlServerSendRtcpPacket(
	  RTC::TransportCongestionControlServer* /*tccServer*/, RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		packet->Serialize(RTC::RTCP::Buffer);

		SendRtcpPacket(packet);
	}

	inline void Transport::OnTimer(Timer* timer)
	{
		MS_TRACE();

		// RTCP timer.
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
