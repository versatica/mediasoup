#define MS_CLASS "RTC::Router"
// #define MS_LOG_DEV

#include "RTC/Router.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
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
			case Channel::Request::MethodId::ROUTER_CLOSE:
			{
#ifdef MS_LOG_DEV
				uint32_t routerId = this->routerId;
#endif

				Destroy();

				MS_DEBUG_DEV("Router closed [routerId:%" PRIu32 "]", routerId);

				request->Accept();

				break;
			}

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
			case Channel::Request::MethodId::TRANSPORT_DUMP:
			case Channel::Request::MethodId::TRANSPORT_SET_REMOTE_DTLS_PARAMETERS:
			case Channel::Request::MethodId::TRANSPORT_SET_MAX_BITRATE:
			case Channel::Request::MethodId::TRANSPORT_CHANGE_UFRAG_PWD:
			case Channel::Request::MethodId::PRODUCER_CLOSE:
			case Channel::Request::MethodId::PRODUCER_DUMP:
			case Channel::Request::MethodId::PRODUCER_RECEIVE:
			case Channel::Request::MethodId::PRODUCER_PAUSE:
			case Channel::Request::MethodId::PRODUCER_RESUME:
			case Channel::Request::MethodId::PRODUCER_SET_RTP_RAW_EVENT:
			case Channel::Request::MethodId::PRODUCER_SET_RTP_OBJECT_EVENT:
			case Channel::Request::MethodId::CONSUMER_CLOSE:
			case Channel::Request::MethodId::CONSUMER_DUMP:
			case Channel::Request::MethodId::CONSUMER_ENABLE:
			case Channel::Request::MethodId::CONSUMER_PAUSE:
			case Channel::Request::MethodId::CONSUMER_RESUME:
			{
				RTC::Peer* peer;

				try
				{
					peer = GetPeerFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (peer == nullptr)
				{
					request->Reject("Peer does not exist");

					return;
				}

				peer->HandleRequest(request);

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

		auto it = this->transports.find(*transportId);
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

		auto it = this->producers.find(*producerId);
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

		auto it = this->consumers.find(*consumerId);
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

		// If the Producer is not in the map, do nothing.
		if (this->mapProducerConsumers.find(producer) == this->mapProducerConsumers.end())
			return;

		// Iterate the map and close all the Consumers associated to it.
		auto& consumers = this->mapProducerConsumers[producer];

		for (auto it = this->consumers.begin(); it != this->consumers.end();)
		{
			auto* consumer = it->second;

			it = consumers.erase(it);
			consumer->Destroy();
		}

		// Finally delete the Producer entry in the map.
		this->mapProducerConsumers.erase(producer);

		// Also delete it from the map of audio levels.
		this->mapProducerAudioLevelContainer.erase(producer);
	}

	void Router::OnProducerRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		    "Producer not present in the map");

		auto& consumers = this->mapProducerConsumers[producer];

		// Send the RtpPacket to all the Consumers associated to the Producer
		// from which it was received.
		for (auto& consumer : consumers)
		{
			// TODO: just if enabled?

			consumer->SendRtpPacket(packet);
		}

		// Update audio levels.
		if (this->audioLevelsEventEnabled)
		{
			uint8_t volume;
			bool voice;

			if (packet->ReadAudioLevel(&volume, &voice))
			{
				int8_t dBov = volume * -1;
				auto& audioLevelContainer = this->mapProducerAudioLevelContainer[producer];

				audioLevelContainer.numdBovs++;
				audioLevelContainer.sumdBovs += dBov;
			}
		}
	}

	void Router::OnConsumerClosed(RTC::Consumer* consumer)
	{
		MS_TRACE();

		// TODO: How to tell the Transport using this Consumer.

		this->consumers.erase(consumer->consumerId);

		for (auto& kv : this->mapProducerConsumers)
		{
			auto& consumers = kv.second;

			consumers.erase(consumer);
		}

		// Finally delete the Consumer entry in the map.
		this->mapConsumerProducer.erase(consumer);
	}

	void Peer::OnConsumerFullFrameRequired(RTC::Consumer* consumer)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapConsumerProducer.find(consumer) != this->mapConsumerProducer.end(),
		    "Consumer not present in the map");

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

				entry.append(Json::UInt{ producer.producerId });
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
