#define MS_CLASS "RTC::Router"
// #define MS_LOG_DEV

#include "RTC/Router.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include "RTC/RtpDictionaries.hpp"
#include <cmath> // std::lround()
#include <set>
#include <string>

namespace RTC
{
	/* Static. */

	static constexpr uint64_t AudioLevelsInterval{ 500 }; // In ms.

	/* Instance methods. */

	Router::Router(Listener* listener, Channel::Notifier* notifier, uint32_t routerId)
	    : routerId(routerId), listener(listener), notifier(notifier)
	{
		MS_TRACE();

		// Set the audio levels timer.
		this->audioLevelsTimer = new Timer(this);
	}

	Router::~Router()
	{
		MS_TRACE();
	}

	void Router::Destroy()
	{
		MS_TRACE();

		// Close all the Producers.
		for (auto it = this->producers.begin(); it != this->producers.end();)
		{
			auto* producer = it->second;

			it = this->producers.erase(it);
			producer->Destroy();
		}

		// Close all the Consumers.
		for (auto it = this->consumers.begin(); it != this->consumers.end();)
		{
			auto* consumer = it->second;

			it = this->consumers.erase(it);
			consumer->Destroy();
		}

		// TODO: Is this warning still true?
		// Close all the Transports.
		// NOTE: It is critical to close Transports after Producers/Consumers
		// because Producer.Destroy() fires an event in the Transport.
		for (auto it = this->transports.begin(); it != this->transports.end();)
		{
			auto* transport = it->second;

			it = this->transports.erase(it);
			transport->Destroy();
		}

		// Close the audio level timer.
		this->audioLevelsTimer->Destroy();

		// Notify the listener.
		this->listener->OnRouterClosed(this);

		delete this;
	}

	Json::Value Router::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringRouterId{ "routerId" };
		static const Json::StaticString JsonStringTransports{ "transports" };
		static const Json::StaticString JsonStringProducers{ "producers" };
		static const Json::StaticString JsonStringConsumers{ "consumers" };
		static const Json::StaticString JsonStringMapProducerConsumers{ "mapProducerConsumers" };
		static const Json::StaticString JsonStringMapConsumerProducer{ "mapConsumerProducer" };
		static const Json::StaticString JsonStringAudioLevelsEventEnabled{ "audioLevelsEventEnabled" };

		Json::Value json(Json::objectValue);
		Json::Value jsonTransports(Json::arrayValue);
		Json::Value jsonProducers(Json::arrayValue);
		Json::Value jsonConsumers(Json::arrayValue);
		Json::Value jsonMapProducerConsumers(Json::objectValue);
		Json::Value jsonMapConsumerProducer(Json::objectValue);

		// Add routerId.
		json[JsonStringRouterId] = Json::UInt{ this->routerId };

		// Add transports.
		for (auto& kv : this->transports)
		{
			auto* transport = kv.second;

			jsonTransports.append(transport->ToJson());
		}
		json[JsonStringTransports] = jsonTransports;

		// Add producers.
		for (auto& kv : this->producers)
		{
			auto* producer = kv.second;

			jsonProducers.append(producer->ToJson());
		}
		json[JsonStringProducers] = jsonProducers;

		// Add consumers.
		for (auto& kv : this->consumers)
		{
			auto* consumer = kv.second;

			jsonConsumers.append(consumer->ToJson());
		}
		json[JsonStringConsumers] = jsonConsumers;

		// Add mapProducerConsumers.
		for (auto& kv : this->mapProducerConsumers)
		{
			auto producer   = kv.first;
			auto& consumers = kv.second;
			Json::Value jsonProducers(Json::arrayValue);

			for (auto& consumer : consumers)
			{
				jsonProducers.append(std::to_string(consumer->consumerId));
			}

			jsonMapProducerConsumers[std::to_string(producer->producerId)] = jsonProducers;
		}
		json[JsonStringMapProducerConsumers] = jsonMapProducerConsumers;

		// Add mapConsumerProducer.
		for (auto& kv : this->mapConsumerProducer)
		{
			auto consumer = kv.first;
			auto producer = kv.second;

			jsonMapConsumerProducer[std::to_string(consumer->consumerId)] =
			    std::to_string(producer->producerId);
		}
		json[JsonStringMapConsumerProducer] = jsonMapConsumerProducer;

		json[JsonStringAudioLevelsEventEnabled] = this->audioLevelsEventEnabled;

		return json;
	}

	void Router::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::ROUTER_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_TRANSPORT:
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

			case Channel::Request::MethodId::ROUTER_CREATE_PRODUCER:
			{
				static const Json::StaticString JsonStringKind{ "kind" };
				static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
				static const Json::StaticString JsonStringRtpMapping{ "rtpMapping" };
				static const Json::StaticString JsonStringPaused{ "paused" };
				static const Json::StaticString JsonStringCodecPayloadTypes{ "codecPayloadTypes" };
				static const Json::StaticString JsonStringHeaderExtensionIds{ "headerExtensionIds" };

				RTC::Producer* producer;
				RTC::Transport* transport;
				uint32_t producerId;

				try
				{
					producer = GetProducerFromRequest(request, &producerId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (producer != nullptr)
				{
					request->Reject("Producer already exists");

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

				if (!request->data[JsonStringKind].isString())
				{
					request->Reject("missing data.kind");

					return;
				}
				else if (!request->data[JsonStringRtpParameters].isObject())
				{
					request->Reject("missing data.rtpParameters");

					return;
				}
				else if (!request->data[JsonStringRtpMapping].isObject())
				{
					request->Reject("missing data.rtpMapping");

					return;
				}

				RTC::Media::Kind kind;
				std::string kindStr = request->data[JsonStringKind].asString();
				RTC::RtpParameters rtpParameters;

				try
				{
					// NOTE: This may throw.
					kind = RTC::Media::GetKind(kindStr);

					// NOTE: This may throw.
					rtpParameters = RTC::RtpParameters(request->data[JsonStringRtpParameters]);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (kind == RTC::Media::Kind::ALL)
					MS_THROW_ERROR("invalid empty kind");

				RTC::Producer::RtpMapping rtpMapping;

				try
				{
					auto& jsonRtpMapping = request->data[JsonStringRtpMapping];

					if (!jsonRtpMapping[JsonStringCodecPayloadTypes].isArray())
						MS_THROW_ERROR("missing rtpMapping.codecPayloadTypes");
					else if (!jsonRtpMapping[JsonStringHeaderExtensionIds].isArray())
						MS_THROW_ERROR("missing rtpMapping.headerExtensionIds");

					for (auto& pair : jsonRtpMapping[JsonStringCodecPayloadTypes])
					{
						if (!pair.isArray() || pair.size() != 2 || !pair[0].isUInt() || !pair[1].isUInt())
							MS_THROW_ERROR("wrong rtpMapping.codecPayloadTypes entry");

						auto sourcePayloadType = static_cast<uint8_t>(pair[0].asUInt());
						auto mappedPayloadType = static_cast<uint8_t>(pair[1].asUInt());

						rtpMapping.codecPayloadTypes[sourcePayloadType] = mappedPayloadType;
					}

					for (auto& pair : jsonRtpMapping[JsonStringHeaderExtensionIds])
					{
						if (!pair.isArray() || pair.size() != 2 || !pair[0].isUInt() || !pair[1].isUInt())
							MS_THROW_ERROR("wrong rtpMapping entry");

						auto sourceHeaderExtensionId = static_cast<uint8_t>(pair[0].asUInt());
						auto mappedHeaderExtensionId = static_cast<uint8_t>(pair[1].asUInt());

						rtpMapping.headerExtensionIds[sourceHeaderExtensionId] = mappedHeaderExtensionId;
					}
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				bool paused = false;

				if (request->data[JsonStringPaused].isBool())
					paused = request->data[JsonStringPaused].asBool();

				// Create a Producer instance.
				producer = new RTC::Producer(
				    this->notifier, producerId, kind, transport, rtpParameters, rtpMapping, paused);

				// Add us as listener.
				producer->AddListener(this);

				try
				{
					// Tell the Transport to handle the new Producer.
					// NOTE: This may throw.
					transport->HandleProducer(producer);
				}
				catch (const MediaSoupError& error)
				{
					delete producer;
					request->Reject(error.what());

					return;
				}

				// Insert into the maps.
				this->producers[producerId] = producer;
				// Ensure the entry will exist even with an empty array.
				this->mapProducerConsumers[producer];

				MS_DEBUG_DEV("Producer created [producerId:%" PRIu32 "]", producerId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_CONSUMER:
			{
				static const Json::StaticString JsonStringKind{ "kind" };
				static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };
				static const Json::StaticString JsonStringPaused{ "paused" };

				RTC::Consumer* consumer;
				RTC::Transport* transport;
				RTC::Producer* producer;
				uint32_t consumerId;

				try
				{
					consumer = GetConsumerFromRequest(request, &consumerId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (consumer != nullptr)
				{
					request->Reject("Consumer already exists");

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

				try
				{
					producer = GetProducerFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (producer == nullptr)
				{
					request->Reject("Producer does not exist");

					return;
				}

				if (!request->data[JsonStringKind].isString())
				{
					request->Reject("missing data.kind");

					return;
				}
				else if (!request->data[JsonStringRtpParameters].isObject())
				{
					request->Reject("missing data.rtpParameters");

					return;
				}

				RTC::Media::Kind kind;
				std::string kindStr = request->data[JsonStringKind].asString();
				RTC::RtpParameters rtpParameters;

				try
				{
					// NOTE: This may throw.
					kind = RTC::Media::GetKind(kindStr);

					// NOTE: This may throw.
					rtpParameters = RTC::RtpParameters(request->data[JsonStringRtpParameters]);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (kind == RTC::Media::Kind::ALL)
					MS_THROW_ERROR("invalid empty kind");
				else if (kind != producer->kind)
					MS_THROW_ERROR("not matching kind");

				bool paused = false;

				if (request->data[JsonStringPaused].isBool())
					paused = request->data[JsonStringPaused].asBool();

				consumer = new RTC::Consumer(
				    this->notifier, consumerId, kind, transport, rtpParameters, paused, producer->producerId);

				// If the Producer is paused tell it to the new Consumer.
				if (producer->IsPaused())
					consumer->SetSourcePaused();

				// Add us as listener.
				consumer->AddListener(this);

				// Tell the Transport to handle the new Consumer.
				transport->HandleConsumer(consumer);

				// Insert into the maps.
				this->consumers[consumerId] = consumer;
				this->mapProducerConsumers[producer].insert(consumer);
				this->mapConsumerProducer[consumer] = producer;

				MS_DEBUG_DEV("Consumer created [consumerId:%" PRIu32 "]", consumerId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::ROUTER_SET_AUDIO_LEVELS_EVENT:
			{
				static const Json::StaticString JsonStringEnabled{ "enabled" };

				if (!request->data[JsonStringEnabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				bool enabled = request->data[JsonStringEnabled].asBool();

				if (enabled == this->audioLevelsEventEnabled)
					return;

				this->audioLevelsEventEnabled = enabled;

				// Clear map of audio levels.
				this->mapProducerAudioLevelContainer.clear();

				// Start or stop audio levels periodic timer.
				if (enabled)
					this->audioLevelsTimer->Start(AudioLevelsInterval, AudioLevelsInterval);
				else
					this->audioLevelsTimer->Stop();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_CLOSE:
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

				transport->Destroy();
				request->Accept();

				break;
			}

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

			case Channel::Request::MethodId::PRODUCER_CLOSE:
			{
				RTC::Producer* producer;

				try
				{
					producer = GetProducerFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (producer == nullptr)
				{
					request->Reject("Producer does not exist");

					return;
				}

				producer->Destroy();
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_DUMP:
			case Channel::Request::MethodId::PRODUCER_PAUSE:
			case Channel::Request::MethodId::PRODUCER_RESUME:
			case Channel::Request::MethodId::PRODUCER_SET_RTP_RAW_EVENT:
			case Channel::Request::MethodId::PRODUCER_SET_RTP_OBJECT_EVENT:
			{
				RTC::Producer* producer;

				try
				{
					producer = GetProducerFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (producer == nullptr)
				{
					request->Reject("Producer does not exist");

					return;
				}

				producer->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::CONSUMER_CLOSE:
			{
				RTC::Consumer* consumer;

				try
				{
					consumer = GetConsumerFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (consumer == nullptr)
				{
					request->Reject("Consumer does not exist");

					return;
				}

				consumer->Destroy();
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_DUMP:
			case Channel::Request::MethodId::CONSUMER_PAUSE:
			case Channel::Request::MethodId::CONSUMER_RESUME:
			{
				RTC::Consumer* consumer;

				try
				{
					consumer = GetConsumerFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (consumer == nullptr)
				{
					request->Reject("Consumer does not exist");

					return;
				}

				consumer->HandleRequest(request);

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	RTC::Transport* Router::GetTransportFromRequest(Channel::Request* request, uint32_t* transportId) const
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

	RTC::Producer* Router::GetProducerFromRequest(Channel::Request* request, uint32_t* producerId) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringProducerId{ "producerId" };

		auto jsonProducerId = request->internal[JsonStringProducerId];

		if (!jsonProducerId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.producerId");

		if (producerId != nullptr)
			*producerId = jsonProducerId.asUInt();

		auto it = this->producers.find(jsonProducerId.asUInt());
		if (it != this->producers.end())
		{
			auto* producer = it->second;

			return producer;
		}

		return nullptr;
	}

	RTC::Consumer* Router::GetConsumerFromRequest(Channel::Request* request, uint32_t* consumerId) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringConsumerId{ "consumerId" };

		auto jsonConsumerId = request->internal[JsonStringConsumerId];

		if (!jsonConsumerId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.consumerId");

		if (consumerId != nullptr)
			*consumerId = jsonConsumerId.asUInt();

		auto it = this->consumers.find(jsonConsumerId.asUInt());
		if (it != this->consumers.end())
		{
			auto* consumer = it->second;

			return consumer;
		}

		return nullptr;
	}

	void Router::OnTransportClosed(RTC::Transport* transport)
	{
		MS_TRACE();

		// TODO: How to tell all the Producers/Consumers using this Transport.
		// Probably using multiple listeners (or not).

		this->transports.erase(transport->transportId);
	}

	void Router::OnProducerClosed(RTC::Producer* producer)
	{
		MS_TRACE();

		// TODO: How to tell the Transport using this Producer.

		this->producers.erase(producer->producerId);

		// Remove the Producer from the map.
		if (this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end())
		{
			// Iterate the map and close all the Consumers associated to it.
			auto& consumers = this->mapProducerConsumers[producer];

			for (auto it = consumers.begin(); it != consumers.end();)
			{
				auto* consumer = *it;

				it = consumers.erase(it);
				consumer->Destroy();
			}

			// Finally delete the Producer entry in the map.
			this->mapProducerConsumers.erase(producer);
		}

		// Also delete it from the map of audio levels.
		this->mapProducerAudioLevelContainer.erase(producer);
	}

	void Router::OnProducerPaused(RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		    "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto& consumer : consumers)
		{
			consumer->SetSourcePaused();
		}
	}

	void Router::OnProducerResumed(RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		    "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto& consumer : consumers)
		{
			consumer->SetSourceResumed();
		}
	}

	void Router::OnProducerRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		    "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		// Send the RtpPacket to all the Consumers associated to the Producer
		// from which it was received.
		for (auto& consumer : consumers)
		{
			consumer->SendRtpPacket(packet);
		}

		// Update audio levels.
		if (this->audioLevelsEventEnabled)
		{
			uint8_t volume;
			bool voice;

			if (packet->ReadAudioLevel(&volume, &voice))
			{
				int8_t dBov               = volume * -1;
				auto& audioLevelContainer = this->mapProducerAudioLevelContainer[producer];

				audioLevelContainer.numdBovs++;
				audioLevelContainer.sumdBovs += dBov;
			}
		}
	}

	void Router::OnConsumerClosed(RTC::Consumer* consumer)
	{
		MS_TRACE();

		this->consumers.erase(consumer->consumerId);

		for (auto& kv : this->mapProducerConsumers)
		{
			auto& consumers = kv.second;

			consumers.erase(consumer);
		}

		// Finally delete the Consumer entry in the map.
		this->mapConsumerProducer.erase(consumer);
	}

	void Router::OnConsumerFullFrameRequired(RTC::Consumer* consumer)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapConsumerProducer.find(consumer) != this->mapConsumerProducer.end(),
		    "Consumer not present in mapConsumerProducer");

		auto& producer = this->mapConsumerProducer[consumer];

		producer->RequestFullFrame();
	}

	inline void Router::OnTimer(Timer* timer)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringEntries{ "entries" };

		// Audio levels timer.
		if (timer == this->audioLevelsTimer)
		{
			Json::Value eventData(Json::objectValue);

			eventData[JsonStringEntries] = Json::arrayValue;

			for (auto& kv : this->mapProducerAudioLevelContainer)
			{
				auto producer             = kv.first;
				auto& audioLevelContainer = kv.second;
				auto numdBovs             = audioLevelContainer.numdBovs;
				auto sumdBovs             = audioLevelContainer.sumdBovs;
				int8_t avgdBov{ -127 };

				if (numdBovs > 0)
					avgdBov = static_cast<int8_t>(std::lround(sumdBovs / numdBovs));
				else
					avgdBov = -127;

				Json::Value entry(Json::arrayValue);

				entry.append(Json::UInt{ producer->producerId });
				entry.append(Json::Int{ avgdBov });

				eventData[JsonStringEntries].append(entry);
			}

			// Clear map.
			this->mapProducerAudioLevelContainer.clear();

			// Emit event.
			this->notifier->Emit(this->routerId, "audiolevels", eventData);
		}
	}
} // namespace RTC
