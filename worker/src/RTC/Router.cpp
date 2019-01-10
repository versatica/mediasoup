#define MS_CLASS "RTC::Router"
// #define MS_LOG_DEV

#include "RTC/Router.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
// #include "RTC/PlainRtpTransport.hpp"
#include "RTC/RtpDictionaries.hpp"
// #include "RTC/WebRtcTransport.hpp"

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
				json data = json::object();

				FillJson(data);

				request->Accept(data);

				break;
			}

				// case Channel::Request::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT:
				// {
				// 	static const Json::StaticString JsonStringUdp{ "udp" };
				// 	static const Json::StaticString JsonStringTcp{ "tcp" };
				// 	static const Json::StaticString JsonStringPreferIPv4{ "preferIPv4" };
				// 	static const Json::StaticString JsonStringPreferIPv6{ "preferIPv6" };
				// 	static const Json::StaticString JsonStringPreferUdp{ "preferUdp" };
				// 	static const Json::StaticString JsonStringPreferTcp{ "preferTcp" };

				// 	std::string transportId;

				// 	try
				// 	{
				// 		SetNewTransportIdFromRequest(request, transportId);
				// 	}
				// 	catch (const MediaSoupError& error)
				// 	{
				// 		request->Reject(error.what());

				// 		return;
				// 	}

				// 	RTC::WebRtcTransport::Options options;

				// 	if (request->data[JsonStringUdp].isBool())
				// 		options.udp = request->data[JsonStringUdp].asBool();
				// 	if (request->data[JsonStringTcp].isBool())
				// 		options.tcp = request->data[JsonStringTcp].asBool();
				// 	if (request->data[JsonStringPreferIPv4].isBool())
				// 		options.preferIPv4 = request->data[JsonStringPreferIPv4].asBool();
				// 	if (request->data[JsonStringPreferIPv6].isBool())
				// 		options.preferIPv6 = request->data[JsonStringPreferIPv6].asBool();
				// 	if (request->data[JsonStringPreferUdp].isBool())
				// 		options.preferUdp = request->data[JsonStringPreferUdp].asBool();
				// 	if (request->data[JsonStringPreferTcp].isBool())
				// 		options.preferTcp = request->data[JsonStringPreferTcp].asBool();

				// 	RTC::WebRtcTransport* webrtcTransport;

				// 	try
				// 	{
				// 		// NOTE: This may throw.
				// 		webrtcTransport = new RTC::WebRtcTransport(this, transportId, options);
				// 	}
				// 	catch (const MediaSoupError& error)
				// 	{
				// 		request->Reject(error.what());

				// 		return;
				// 	}

				// 	// Insert into the map.
				// 	this->mapTransports[transportId] = webrtcTransport;

				// 	MS_DEBUG_DEV("WebRtcTransport created [transportId:%s]", transportId.c_str());

				// 	json data = json::object();

				// 	webrtcTransport->FillJson(data);

				// 	request->Accept(data);

				// 	break;
				// }

				// case Channel::Request::MethodId::ROUTER_CREATE_PLAIN_RTP_TRANSPORT:
				// {
				// 	static const Json::StaticString JsonStringRemoteIp{ "remoteIp" };
				// 	static const Json::StaticString JsonStringRemotePort{ "remotePort" };
				// 	static const Json::StaticString JsonStringLocalIp{ "localIp" };
				// 	static const Json::StaticString JsonStringPreferIPv4{ "preferIPv4" };
				// 	static const Json::StaticString JsonStringPreferIPv6{ "preferIPv6" };

				// 	std::string transportId;

				// 	try
				// 	{
				// 		SetNewTransportIdFromRequest(request, transportId);
				// 	}
				// 	catch (const MediaSoupError& error)
				// 	{
				// 		request->Reject(error.what());

				// 		return;
				// 	}

				// 	RTC::PlainRtpTransport::Options options;

				// 	if (request->data[JsonStringRemoteIp].isString())
				// 		options.remoteIp = request->data[JsonStringRemoteIp].asString();

				// 	if (request->data[JsonStringRemotePort].isUInt())
				// 		options.remotePort = request->data[JsonStringRemotePort].asUInt();

				// 	if (request->data[JsonStringLocalIp].isString())
				// 		options.localIp = request->data[JsonStringLocalIp].asString();

				// 	if (request->data[JsonStringPreferIPv4].isBool())
				// 		options.preferIPv4 = request->data[JsonStringPreferIPv4].asBool();

				// 	if (request->data[JsonStringPreferIPv6].isBool())
				// 		options.preferIPv6 = request->data[JsonStringPreferIPv6].asBool();

				// 	RTC::PlainRtpTransport* plainRtpTransport;

				// 	try
				// 	{
				// 		// NOTE: This may throw.
				// 		plainRtpTransport = new RTC::PlainRtpTransport(this, transportId, options);
				// 	}
				// 	catch (const MediaSoupError& error)
				// 	{
				// 		request->Reject(error.what());

				// 		return;
				// 	}

				// 	// Insert into the map.
				// 	this->mapTransports[transportId] = plainRtpTransport;

				// 	MS_DEBUG_DEV("PlainRtpTransport created [transportId:%s]", transportId.c_str());

				// 	json data = json::object();

				// 	plainRtpTransport->FillJson(data);

				// 	request->Accept(data);

				// 	break;
				// }

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

				// Call transport->Close() so it will notify us about its closed Producers
				// and Consumers.
				transport->Close();

				// Remove it from the map and delete it.
				this->mapTransports.erase(transport->id);
				delete transport;

				MS_DEBUG_DEV("Transport closed [id:%s]", transport->id.c_str());

				request->Accept();

				break;
			}

			default:
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
			// Call consumer->ProducerClosed() so it will notify the Node process,
			// will notify its Transport, and its Transport will delete the Consumer.
			consumer->ProducerClosed();
		}

		// Remove the the Producer from the maps.
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

	void Router::OnTransportProducerStreamEnabled(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  const RTC::RtpStream* rtpStream,
	  uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->EnableStream(rtpStream, mappedSsrc);
		}
	}

	void Router::OnTransportProducerStreamDisabled(
	  RTC::Transport* /*transport*/,
	  RTC::Producer* producer,
	  const RTC::RtpStream* rtpStream,
	  uint32_t mappedSsrc)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers.at(producer);

		for (auto* consumer : consumers)
		{
			consumer->DisableStream(rtpStream, mappedSsrc);
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
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer, uint32_t ssrc)
	{
		MS_TRACE();

		auto mapConsumerProducerIt = this->mapConsumerProducer.find(consumer);

		MS_ASSERT(
		  mapConsumerProducerIt != this->mapConsumerProducer.end(),
		  "Consumer not present in mapConsumerProducer");

		// Get the associated Producer.
		auto* producer = mapConsumerProducerIt->second;

		producer->RequestKeyFrame(ssrc);
	}
} // namespace RTC
