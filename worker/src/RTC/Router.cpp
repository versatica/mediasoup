#define MS_CLASS "RTC::Router"
// #define MS_LOG_DEV

#include "RTC/Router.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/PlainRtpTransport.hpp"
#include "RTC/RtpDictionaries.hpp"
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

		// Clear other maps.
		this->mapProducerConsumers.clear();
		this->mapConsumerProducer.clear();
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
				jsonProducerIdIt->emplace_back(consumer->id)
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
	}

	void Router::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::ROUTER_DUMP:
			{
				json data{ json::object() };

				FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT:
			{
				std::string transportId;

				// This may throw.
				SetNewTransportIdFromRequest(request, transportId);

				RTC::WebRtcTransport::Options options;

				auto jsonListenIpsIt = request->data.find("listenIps");

				if (jsonListenIpsIt == request->data.end())
					MS_THROW_TYPE_ERROR("missing listenIps");
				else if (!jsonListenIpsIt->is_array())
					MS_THROW_TYPE_ERROR("wrong listenIps (not an array)");
				else if (jsonListenIpsIt->size() == 0)
					MS_THROW_TYPE_ERROR("wrong listenIps (empty array)");
				else if (jsonListenIpsIt->size() > 8)
					MS_THROW_TYPE_ERROR("wrong listenIps (too many IPs)");

				for (auto& jsonListenIp : *jsonListenIpsIt)
				{
					RTC::WebRtcTransport::ListenIp listenIp;

					if (!jsonListenIp.is_object())
						MS_THROW_TYPE_ERROR("wrong listenIp (not an object)");

					auto jsonIpIt = jsonListenIp.find("ip");

					if (jsonIpIt == jsonListenIp.end())
						MS_THROW_TYPE_ERROR("missing listenIp.ip");
					else if (!jsonIpIt->is_string())
						MS_THROW_TYPE_ERROR("wrong listenIp.ip (not an string");

					// This may throw.
					listenIp.ip = Utils::IP::NormalizeIp(jsonIpIt->get<std::string>());

					auto jsonAnnouncedIpIt = jsonListenIp.find("announcedIp");

					if (jsonAnnouncedIpIt != jsonListenIp.end())
					{
						if (!jsonAnnouncedIpIt->is_string())
							MS_THROW_TYPE_ERROR("wrong listenIp.announcedIp (not an string)");

						listenIp.announcedIp = jsonAnnouncedIpIt->get<std::string>();
					}

					options.listenIps.push_back(listenIp);
				}

				auto jsonEnableUdpIt = request->data.find("enableUdp");

				if (jsonEnableUdpIt != request->data.end())
				{
					if (!jsonEnableUdpIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong enableUdp (not a boolean)");

					options.enableUdp = jsonEnableUdpIt->get<bool>();
				}

				auto jsonEnableTcpIt = request->data.find("enableTcp");

				if (jsonEnableTcpIt != request->data.end())
				{
					if (!jsonEnableTcpIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong enableTcp (not a boolean)");

					options.enableTcp = jsonEnableTcpIt->get<bool>();
				}

				auto jsonPreferUdpIt = request->data.find("preferUdp");

				if (jsonPreferUdpIt != request->data.end())
				{
					if (!jsonPreferUdpIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong preferUdp (not a boolean)");

					options.preferUdp = jsonPreferUdpIt->get<bool>();
				}

				auto jsonPreferTcpIt = request->data.find("preferTcp");

				if (jsonPreferTcpIt != request->data.end())
				{
					if (!jsonPreferTcpIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong preferTcp (not a boolean)");

					options.preferTcp = jsonPreferTcpIt->get<bool>();
				}

				// This may throw.
				RTC::WebRtcTransport* webrtcTransport = new RTC::WebRtcTransport(transportId, this, options);

				// Insert into the map.
				this->mapTransports[transportId] = webrtcTransport;

				MS_DEBUG_DEV("WebRtcTransport created [transportId:%s]", transportId.c_str());

				json data{ json::object() };

				webrtcTransport->FillJson(data);

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_PLAIN_RTP_TRANSPORT:
			{
				std::string transportId;

				// This may throw
				SetNewTransportIdFromRequest(request, transportId);

				RTC::PlainRtpTransport::Options options;

				auto jsonListenIpIt = request->data.find("listenIp");

				if (jsonListenIpIt == request->data.end())
					MS_THROW_TYPE_ERROR("missing listenIp");
				else if (!jsonListenIpIt->is_object())
					MS_THROW_TYPE_ERROR("wrong listenIp (not an object)");

				auto jsonIpIt = jsonListenIpIt->find("ip");

				if (jsonIpIt == jsonListenIpIt->end())
					MS_THROW_TYPE_ERROR("missing listenIp.ip");
				else if (!jsonIpIt->is_string())
					MS_THROW_TYPE_ERROR("wrong listenIp.ip (not an string)");

				// This may throw.
				options.listenIp.ip = Utils::IP::NormalizeIp(jsonIpIt->get<std::string>());

				auto jsonAnnouncedIpIt = jsonListenIpIt->find("announcedIp");

				if (jsonAnnouncedIpIt != jsonListenIpIt->end())
				{
					if (!jsonAnnouncedIpIt->is_string())
						MS_THROW_TYPE_ERROR("wrong listenIp.announcedIp (not an string");

					options.listenIp.announcedIp = jsonAnnouncedIpIt->get<std::string>();
				}

				auto jsonRtcpMuxIt = request->data.find("rtcpMux");

				if (jsonRtcpMuxIt != request->data.end())
				{
					if (!jsonRtcpMuxIt->is_boolean())
						MS_THROW_TYPE_ERROR("wrong rtcpMux (not a boolean)");

					options.rtcpMux = jsonRtcpMuxIt->get<bool>();
				}

				RTC::PlainRtpTransport* plainRtpTransport =
				  new RTC::PlainRtpTransport(transportId, this, options);

				// Insert into the map.
				this->mapTransports[transportId] = plainRtpTransport;

				MS_DEBUG_DEV("PlainRtpTransport created [transportId:%s]", transportId.c_str());

				json data{ json::object() };

				plainRtpTransport->FillJson(data);

				request->Accept(data);

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
				delete transport;

				MS_DEBUG_DEV("Transport closed [id:%s]", transport->id.c_str());

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

	void Router::OnTransportNewProducer(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) == this->mapProducerConsumers.end(),
		  "Producer already present in mapProducerConsumers");
		MS_ASSERT(
		  this->mapProducers.find(producer->id) == this->mapProducers.end(),
		  "Producer already present in mapProducers");

		// Insert the Producer in the maps.
		this->mapProducerConsumers[producer];
		this->mapProducers[producer->id] = producer;
	}

	void Router::OnTransportProducerClosed(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto mapProducerConsumersIt = this->mapProducerConsumers.find(producer);
		auto mapProducersIt         = this->mapProducers.find(producer->id);

		MS_ASSERT(
		  mapProducerConsumersIt != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");
		MS_ASSERT(mapProducersIt != this->mapProducers.end(), "Producer not present in mapProducers");

		// Close all Consumers associated to the closed Producer.
		auto& consumers = mapProducerConsumersIt->second;

		for (auto* consumer : consumers)
		{
			// Call consumer->ProducerClosed() so the Consumer will notify the Node process,
			// will notify its Transport, and its Transport will delete the Consumer.
			consumer->ProducerClosed();
		}

		// Remove the Producer from the maps.
		this->mapProducerConsumers.erase(mapProducerConsumersIt);
		this->mapProducers.erase(mapProducersIt);
	}

	void Router::OnTransportProducerPaused(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerPaused();
		}
	}

	void Router::OnTransportProducerResumed(RTC::Transport* /*transport*/, RTC::Producer* producer)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerResumed();
		}
	}

	void Router::OnTransportProducerRtpStreamHealthy(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  const RTC::RtpStream* rtpStream,
	  uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerRtpStreamHealthy(rtpStream, mappedSsrc);
		}
	}

	void Router::OnTransportProducerRtpStreamUnhealthy(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  const RTC::RtpStream* rtpStream,
	  uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->ProducerRtpStreamUnhealthy(rtpStream, mappedSsrc);
		}
	}

	void Router::OnTransportProducerRtpPacketReceived(
	  RTC::Transport* /*transport*/, RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->SendRtpPacket(packet);
		}
	}

	const RTC::Producer* Router::OnTransportGetProducer(
	  RTC::Transport* /*transport*/, std::string& producerId)
	{
		MS_TRACE();

		auto mapProducersIt = this->mapProducers.find(producerId);

		if (mapProducersIt == this->mapProducers.end())
			return nullptr;

		auto* producer = mapProducersIt->second;

		return producer;
	}

	void Router::OnTransportNewConsumer(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer, const RTC::Producer* producer)
	{
		MS_TRACE();

		auto mapProducerConsumersIt = this->mapProducerConsumers.find(producer);

		MS_ASSERT(
		  mapProducerConsumersIt != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");
		MS_ASSERT(
		  this->mapConsumerProducer.find(consumer) == this->mapConsumerProducer.end(),
		  "Consumer already present in mapConsumerProducer");

		auto& consumers = mapProducerConsumersIt->second;

		// Insert the Consumer in the maps.
		consumers.insert(consumer);
		this->mapConsumerProducer[consumer] = producer;
	}

	void Router::OnTransportConsumerClosed(RTC::Transport* /*transport*/, RTC::Consumer* consumer)
	{
		MS_TRACE();

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

	void Router::OnTransportConsumerKeyFrameRequested(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer, uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto mapConsumerProducerIt = this->mapConsumerProducer.find(consumer);
		auto* producer             = mapConsumerProducerIt->second;

		producer->RequestKeyFrame(mappedSsrc);
	}
} // namespace RTC
