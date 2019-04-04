#define MS_CLASS "RTC::Router"
// #define MS_LOG_DEV

#include "RTC/Router.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/AudioLevelObserver.hpp"
#include "RTC/PipeTransport.hpp"
#include "RTC/PlainRtpTransport.hpp"
#include "RTC/WebRtcTransport.hpp"

namespace RTC
{
	/* Instance methods. */

	Router::Router(const std::string& id) : id(id)
	{
		MS_TRACE();
	}

	Router::~Router()
	{
		MS_TRACE();

		// Close all Transports.
		for (auto& kv : this->mapTransports)
		{
			auto* transport = kv.second;

			delete transport;
		}
		this->mapTransports.clear();

		// Close all RtpObservers.
		for (auto& kv : this->mapRtpObservers)
		{
			auto* rtpObserver = kv.second;

			delete rtpObserver;
		}
		this->mapRtpObservers.clear();

		// Clear other maps.
		this->mapProducerConsumers.clear();
		this->mapConsumerProducer.clear();
		this->mapProducerRtpObservers.clear();
		this->mapProducers.clear();
	}

	void Router::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add transportIds.
		jsonObject["transportIds"] = json::array();
		auto jsonTransportIdsIt    = jsonObject.find("transportIds");

		for (auto& kv : this->mapTransports)
		{
			auto& transportId = kv.first;

			jsonTransportIdsIt->emplace_back(transportId);
		}

		// Add rtpObserverIds.
		jsonObject["rtpObserverIds"] = json::array();
		auto jsonRtpObserverIdsIt    = jsonObject.find("rtpObserverIds");

		for (auto& kv : this->mapRtpObservers)
		{
			auto& rtpObserverId = kv.first;

			jsonRtpObserverIdsIt->emplace_back(rtpObserverId);
		}

		// Add mapProducerIdConsumerIds.
		jsonObject["mapProducerIdConsumerIds"] = json::object();
		auto jsonMapProducerConsumersIt        = jsonObject.find("mapProducerIdConsumerIds");

		for (auto& kv : this->mapProducerConsumers)
		{
			auto* producer  = kv.first;
			auto& consumers = kv.second;

			(*jsonMapProducerConsumersIt)[producer->id] = json::array();
			auto jsonProducerIdIt                       = jsonMapProducerConsumersIt->find(producer->id);

			for (auto* consumer : consumers)
			{
				jsonProducerIdIt->emplace_back(consumer->id);
			}
		}

		// Add mapConsumerIdProducerId.
		jsonObject["mapConsumerIdProducerId"] = json::object();
		auto jsonMapConsumerProducerIt        = jsonObject.find("mapConsumerIdProducerId");

		for (auto& kv : this->mapConsumerProducer)
		{
			auto* consumer = kv.first;
			auto* producer = kv.second;

			(*jsonMapConsumerProducerIt)[consumer->id] = producer->id;
		}

		// Add mapProducerIdObserverIds.
		jsonObject["mapProducerIdObserverIds"] = json::object();
		auto jsonMapProducerRtpObserversIt     = jsonObject.find("mapProducerIdObserverIds");

		for (auto& kv : this->mapProducerRtpObservers)
		{
			auto* producer     = kv.first;
			auto& rtpObservers = kv.second;

			(*jsonMapProducerRtpObserversIt)[producer->id] = json::array();
			auto jsonProducerIdIt = jsonMapProducerRtpObserversIt->find(producer->id);

			for (auto* rtpObserver : rtpObservers)
			{
				jsonProducerIdIt->emplace_back(rtpObserver->id);
			}
		}
	}

	void Router::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::ROUTER_DUMP:
			{
				json data(json::object());

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT:
			{
				std::string transportId;

				// This may throw.
				SetNewTransportIdFromRequest(request, transportId);

				// This may throw.
				auto* webRtcTransport = new RTC::WebRtcTransport(transportId, this, request->data);

				// Insert into the map.
				this->mapTransports[transportId] = webRtcTransport;

				MS_DEBUG_DEV("WebRtcTransport created [transportId:%s]", transportId.c_str());

				json data(json::object());

				webRtcTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_PLAIN_RTP_TRANSPORT:
			{
				std::string transportId;

				// This may throw
				SetNewTransportIdFromRequest(request, transportId);

				auto* plainRtpTransport = new RTC::PlainRtpTransport(transportId, this, request->data);

				// Insert into the map.
				this->mapTransports[transportId] = plainRtpTransport;

				MS_DEBUG_DEV("PlainRtpTransport created [transportId:%s]", transportId.c_str());

				json data(json::object());

				plainRtpTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_PIPE_TRANSPORT:
			{
				std::string transportId;

				// This may throw
				SetNewTransportIdFromRequest(request, transportId);

				auto* pipeTransport = new RTC::PipeTransport(transportId, this, request->data);

				// Insert into the map.
				this->mapTransports[transportId] = pipeTransport;

				MS_DEBUG_DEV("PipeTransport created [transportId:%s]", transportId.c_str());

				json data(json::object());

				pipeTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_AUDIO_LEVEL_OBSERVER:
			{
				std::string rtpObserverId;

				// This may throw
				SetNewRtpObserverIdFromRequest(request, rtpObserverId);

				auto* audioLevelObserver = new RTC::AudioLevelObserver(rtpObserverId, request->data);

				// Insert into the map.
				this->mapRtpObservers[rtpObserverId] = audioLevelObserver;

				MS_DEBUG_DEV("AudioLevelObserver created [rtpObserverId:%s]", rtpObserverId.c_str());

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_CLOSE:
			{
				// This may throw.
				RTC::Transport* transport = GetTransportFromRequest(request);

				// Tell the Transport to close all its Producers and Consumers so it will
				// notify us about their closured.
				transport->CloseProducersAndConsumers();

				// Remove it from the map and delete it.
				this->mapTransports.erase(transport->id);

				MS_DEBUG_DEV("Transport closed [transportId:%s]", transport->id.c_str());

				// Delete it.
				delete transport;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_OBSERVER_CLOSE:
			{
				// This may throw.
				RTC::RtpObserver* rtpObserver = GetRtpObserverFromRequest(request);

				// Remove it from the map.
				this->mapRtpObservers.erase(rtpObserver->id);

				// Iterate all entries in mapProducerRtpObservers and remove the closed one.
				for (auto& kv : this->mapProducerRtpObservers)
				{
					auto& rtpObservers = kv.second;

					rtpObservers.erase(rtpObserver);
				}

				MS_DEBUG_DEV("RtpObserver closed [rtpObserverId:%s]", rtpObserver->id.c_str());

				// Delete it.
				delete rtpObserver;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_OBSERVER_PAUSE:
			{
				// This may throw.
				RTC::RtpObserver* rtpObserver = GetRtpObserverFromRequest(request);

				rtpObserver->Pause();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_OBSERVER_RESUME:
			{
				// This may throw.
				RTC::RtpObserver* rtpObserver = GetRtpObserverFromRequest(request);

				rtpObserver->Resume();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_OBSERVER_ADD_PRODUCER:
			{
				// This may throw.
				RTC::RtpObserver* rtpObserver = GetRtpObserverFromRequest(request);
				RTC::Producer* producer       = GetProducerFromRequest(request);

				rtpObserver->AddProducer(producer);

				// Add to the map.
				this->mapProducerRtpObservers[producer].insert(rtpObserver);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::RTP_OBSERVER_REMOVE_PRODUCER:
			{
				// This may throw.
				RTC::RtpObserver* rtpObserver = GetRtpObserverFromRequest(request);
				RTC::Producer* producer       = GetProducerFromRequest(request);

				rtpObserver->RemoveProducer(producer);

				// Remove from the map.
				this->mapProducerRtpObservers[producer].erase(rtpObserver);

				request->Accept();

				break;
			}

			// Any other request must be delivered to the corresponding Transport.
			default:
			{
				// This may throw.
				RTC::Transport* transport = GetTransportFromRequest(request);

				transport->HandleRequest(request);

				break;
			}
		}
	}

	void Router::SetNewTransportIdFromRequest(Channel::Request* request, std::string& transportId) const
	{
		MS_TRACE();

		auto jsonTransportIdIt = request->internal.find("transportId");

		if (jsonTransportIdIt == request->internal.end() || !jsonTransportIdIt->is_string())
			MS_THROW_ERROR("request has no internal.transportId");

		transportId.assign(jsonTransportIdIt->get<std::string>());

		if (this->mapTransports.find(transportId) != this->mapTransports.end())
			MS_THROW_ERROR("a Transport with same transportId already exists");
	}

	RTC::Transport* Router::GetTransportFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		auto jsonTransportIdIt = request->internal.find("transportId");

		if (jsonTransportIdIt == request->internal.end() || !jsonTransportIdIt->is_string())
			MS_THROW_ERROR("request has no internal.transportId");

		auto it = this->mapTransports.find(jsonTransportIdIt->get<std::string>());

		if (it == this->mapTransports.end())
			MS_THROW_ERROR("Transport not found");

		RTC::Transport* transport = it->second;

		return transport;
	}

	void Router::SetNewRtpObserverIdFromRequest(Channel::Request* request, std::string& rtpObserverId) const
	{
		MS_TRACE();

		auto jsonRtpObserverIdIt = request->internal.find("rtpObserverId");

		if (jsonRtpObserverIdIt == request->internal.end() || !jsonRtpObserverIdIt->is_string())
			MS_THROW_ERROR("request has no internal.rtpObserverId");

		rtpObserverId.assign(jsonRtpObserverIdIt->get<std::string>());

		if (this->mapRtpObservers.find(rtpObserverId) != this->mapRtpObservers.end())
			MS_THROW_ERROR("an RtpObserver with same rtpObserverId already exists");
	}

	RTC::RtpObserver* Router::GetRtpObserverFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		auto jsonRtpObserverIdIt = request->internal.find("rtpObserverId");

		if (jsonRtpObserverIdIt == request->internal.end() || !jsonRtpObserverIdIt->is_string())
			MS_THROW_ERROR("request has no internal.rtpObserverId");

		auto it = this->mapRtpObservers.find(jsonRtpObserverIdIt->get<std::string>());

		if (it == this->mapRtpObservers.end())
			MS_THROW_ERROR("RtpObserver not found");

		RTC::RtpObserver* rtpObserver = it->second;

		return rtpObserver;
	}

	RTC::Producer* Router::GetProducerFromRequest(Channel::Request* request) const
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

	inline void Router::OnTransportNewProducer(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) == this->mapProducerConsumers.end(),
		  "Producer already present in mapProducerConsumers");

		if (this->mapProducers.find(producer->id) != this->mapProducers.end())
		{
			MS_THROW_ERROR("Producer already present in mapProducers [producerId:%s]", producer->id.c_str());
		}

		// Insert the Producer in the maps.
		this->mapProducers[producer->id] = producer;
		this->mapProducerConsumers[producer];
		this->mapProducerRtpObservers[producer];
	}

	inline void Router::OnTransportProducerClosed(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto mapProducerConsumersIt    = this->mapProducerConsumers.find(producer);
		auto mapProducersIt            = this->mapProducers.find(producer->id);
		auto mapProducerRtpObserversIt = this->mapProducerRtpObservers.find(producer);

		MS_ASSERT(
		  mapProducerConsumersIt != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");
		MS_ASSERT(mapProducersIt != this->mapProducers.end(), "Producer not present in mapProducers");
		MS_ASSERT(
		  mapProducerRtpObserversIt != this->mapProducerRtpObservers.end(),
		  "Producer not present in mapProducerRtpObservers");

		// Close all Consumers associated to the closed Producer.
		auto& consumers = mapProducerConsumersIt->second;

		// NOTE: While iterating the set of Consumers, we call ProducerClosed() on each
		// one, which will end calling Router::OnTransportConsumerProducerClosed(),
		// which will remove the Consumer from mapConsumerProducer but won't remove the
		// closed Consumer from the set of Consumers in mapProducerConsumers (here will
		// erase the complete entry in that map).
		for (auto* consumer : consumers)
		{
			// Call consumer->ProducerClosed() so the Consumer will notify the Node process,
			// will notify its Transport, and its Transport will delete the Consumer.
			consumer->ProducerClosed();
		}

		// Tell all RtpObservers that the Producer has been closed.
		auto& rtpObservers = mapProducerRtpObserversIt->second;

		for (auto* rtpObserver : rtpObservers)
		{
			rtpObserver->RemoveProducer(producer);
		}

		// Remove the Producer from the maps.
		this->mapProducers.erase(mapProducersIt);
		this->mapProducerConsumers.erase(mapProducerConsumersIt);
		this->mapProducerRtpObservers.erase(mapProducerRtpObserversIt);
	}

	inline void Router::OnTransportProducerPaused(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerPaused();
		}

		auto it = this->mapProducerRtpObservers.find(producer);

		if (it != this->mapProducerRtpObservers.end())
		{
			auto& rtpObservers = it->second;

			for (auto* rtpObserver : rtpObservers)
			{
				rtpObserver->ProducerPaused(producer);
			}
		}
	}

	inline void Router::OnTransportProducerResumed(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerResumed();
		}

		auto it = this->mapProducerRtpObservers.find(producer);

		if (it != this->mapProducerRtpObservers.end())
		{
			auto& rtpObservers = it->second;

			for (auto* rtpObserver : rtpObservers)
			{
				rtpObserver->ProducerResumed(producer);
			}
		}
	}

	inline void Router::OnTransportProducerNewRtpStream(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  RTC::RtpStreamRecv* rtpStream,
	  uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerNewRtpStream(rtpStream, mappedSsrc);
		}
	}

	inline void Router::OnTransportProducerRtpStreamScore(
	  RTC::Transport* /*transport*/, RTC::Producer* producer, RTC::RtpStreamRecv* rtpStream, uint8_t score)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerRtpStreamScore(rtpStream, score);
		}
	}

	inline void Router::OnTransportProducerRtpPacketReceived(
	  RTC::Transport* /*transport*/, RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->SendRtpPacket(packet);
		}

		auto it = this->mapProducerRtpObservers.find(producer);

		if (it != this->mapProducerRtpObservers.end())
		{
			auto& rtpObservers = it->second;

			for (auto* rtpObserver : rtpObservers)
			{
				rtpObserver->ReceiveRtpPacket(producer, packet);
			}
		}
	}

	inline void Router::OnTransportNeedWorstRemoteFractionLost(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  uint32_t mappedSsrc,
	  uint8_t& worstRemoteFractionLost)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->NeedWorstRemoteFractionLost(mappedSsrc, worstRemoteFractionLost);
		}
	}

	inline void Router::OnTransportNewConsumer(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer, std::string& producerId)
	{
		MS_TRACE();

		auto mapProducersIt = this->mapProducers.find(producerId);

		if (mapProducersIt == this->mapProducers.end())
			MS_THROW_ERROR("Producer not found [producerId:%s]", producerId.c_str());

		auto* producer              = mapProducersIt->second;
		auto mapProducerConsumersIt = this->mapProducerConsumers.find(producer);

		MS_ASSERT(
		  mapProducerConsumersIt != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");
		MS_ASSERT(
		  this->mapConsumerProducer.find(consumer) == this->mapConsumerProducer.end(),
		  "Consumer already present in mapConsumerProducer");

		// Update the Consumer status based on the Producer status.
		if (producer->IsPaused())
			consumer->ProducerPaused();

		// Insert the Consumer in the maps.
		auto& consumers = mapProducerConsumersIt->second;

		consumers.insert(consumer);
		this->mapConsumerProducer[consumer] = producer;

		// Get all streams in the Producer and provide the Consumer with them.
		for (auto& kv : producer->GetRtpStreams())
		{
			auto* rtpStream     = kv.first;
			uint32_t mappedSsrc = kv.second;

			consumer->ProducerRtpStream(rtpStream, mappedSsrc);
		}
	}

	inline void Router::OnTransportConsumerClosed(RTC::Transport* /*transport*/, RTC::Consumer* consumer)
	{
		MS_TRACE();

		// NOTE:
		// This callback is called when the Consumer has been closed but its Produer
		// remains alive, so the entry in mapProducerConsumers still exists and must
		// be removed.

		auto mapConsumerProducerIt = this->mapConsumerProducer.find(consumer);

		MS_ASSERT(
		  mapConsumerProducerIt != this->mapConsumerProducer.end(),
		  "Consumer not present in mapConsumerProducer");

		// Get the associated Producer.
		auto* producer = mapConsumerProducerIt->second;

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");

		// Remove the Consumer from the set of Consumers of the Producer.
		auto& consumers = this->mapProducerConsumers.at(producer);

		consumers.erase(consumer);

		// Remove the Consumer from the map.
		this->mapConsumerProducer.erase(mapConsumerProducerIt);
	}

	inline void Router::OnTransportConsumerProducerClosed(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer)
	{
		MS_TRACE();

		// NOTE:
		// This callback is called when the Consumer has been closed because its
		// Producer was closed, so the entry in mapProducerConsumers has already been
		// removed.

		auto mapConsumerProducerIt = this->mapConsumerProducer.find(consumer);

		MS_ASSERT(
		  mapConsumerProducerIt != this->mapConsumerProducer.end(),
		  "Consumer not present in mapConsumerProducer");

		// Remove the Consumer from the map.
		this->mapConsumerProducer.erase(mapConsumerProducerIt);
	}

	inline void Router::OnTransportConsumerKeyFrameRequested(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto* producer = this->mapConsumerProducer.at(consumer);

		producer->RequestKeyFrame(mappedSsrc);
	}
} // namespace RTC
