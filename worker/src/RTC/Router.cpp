#define MS_CLASS "RTC::Router"
// #define MS_LOG_DEV

#include "RTC/Router.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include "Channel/Notifier.hpp"
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

		// Close all the Producers.
		for (auto it = this->producers.begin(); it != this->producers.end();)
		{
			auto* producer = it->second;

			it = this->producers.erase(it);
			delete producer;
		}

		// Close all the Consumers.
		for (auto it = this->consumers.begin(); it != this->consumers.end();)
		{
			auto* consumer = it->second;

			it = this->consumers.erase(it);
			delete consumer;
		}

		// Close all the Transports.
		// NOTE: It is critical to close Transports after Producers/Consumers
		// because their destructor fires an event in the Transport.
		for (auto it = this->transports.begin(); it != this->transports.end();)
		{
			auto* transport = it->second;

			it = this->transports.erase(it);
			delete transport;
		}
	}

	void Router::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add id.
		jsonObject["id"] = this->id;

		// Add transportIds.
		jsonObject["transportIds"] = json::array();
		auto jsonTransportIdsIt    = jsonObject.find("transportIds");

		for (auto& kv : this->transports)
		{
			auto& transportId = kv.first;

			jsonTransportIdsIt->emplace_back(transportId);
		}

		// Add mapProducerConsumers.
		jsonObject["mapProducerConsumers"] = json::object();
		auto jsonMapProducerConsumersIt    = jsonObject.find("mapProducerConsumers");

		for (auto& kv : this->mapProducerConsumers)
		{
			auto* producer  = kv.first;
			auto& consumers = kv.second;

			(*jsonMapProducerConsumersIt)[producer.id] = json::array();
			auto jsonProducerIdIt = jsonMapProducerConsumersIt->find(producer.id);

			for (auto* consumer : consumers)
			{
				jsonProducerIdIt->emplace_back(consumer.id)
			}
		}

		// Add mapConsumerProducer.
		jsonObject["mapConsumerProducer"] = json::object();
		auto jsonMapConsumerProducerIt    = jsonObject.find("mapConsumerProducer");

		for (auto& kv : this->mapConsumerProducer)
		{
			auto* consumer = kv.first;
			auto* producer = kv.second;

			(*jsonMapConsumerProducerIt)[consumer.id] = producer.id;
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

			case Channel::Request::MethodId::ROUTER_CREATE_WEBRTC_TRANSPORT:
			{
				static const Json::StaticString JsonStringUdp{ "udp" };
				static const Json::StaticString JsonStringTcp{ "tcp" };
				static const Json::StaticString JsonStringPreferIPv4{ "preferIPv4" };
				static const Json::StaticString JsonStringPreferIPv6{ "preferIPv6" };
				static const Json::StaticString JsonStringPreferUdp{ "preferUdp" };
				static const Json::StaticString JsonStringPreferTcp{ "preferTcp" };

				std::string transportId;

				try
				{
					SetNewTransportIdFromRequest(request, transportId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				RTC::WebRtcTransport::Options options;

				if (request->data[JsonStringUdp].isBool())
					options.udp = request->data[JsonStringUdp].asBool();
				if (request->data[JsonStringTcp].isBool())
					options.tcp = request->data[JsonStringTcp].asBool();
				if (request->data[JsonStringPreferIPv4].isBool())
					options.preferIPv4 = request->data[JsonStringPreferIPv4].asBool();
				if (request->data[JsonStringPreferIPv6].isBool())
					options.preferIPv6 = request->data[JsonStringPreferIPv6].asBool();
				if (request->data[JsonStringPreferUdp].isBool())
					options.preferUdp = request->data[JsonStringPreferUdp].asBool();
				if (request->data[JsonStringPreferTcp].isBool())
					options.preferTcp = request->data[JsonStringPreferTcp].asBool();

				RTC::WebRtcTransport* webrtcTransport;

				try
				{
					// NOTE: This may throw.
					webrtcTransport = new RTC::WebRtcTransport(this, transportId, options);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// Insert into the map.
				this->transports[transportId] = webrtcTransport;

				MS_DEBUG_DEV("WebRtcTransport created [transportId:%" PRIu32 "]", transportId);

				auto data = webrtcTransport->ToJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::ROUTER_CREATE_PLAIN_RTP_TRANSPORT:
			{
				static const Json::StaticString JsonStringRemoteIp{ "remoteIp" };
				static const Json::StaticString JsonStringRemotePort{ "remotePort" };
				static const Json::StaticString JsonStringLocalIp{ "localIp" };
				static const Json::StaticString JsonStringPreferIPv4{ "preferIPv4" };
				static const Json::StaticString JsonStringPreferIPv6{ "preferIPv6" };

				std::string transportId;

				try
				{
					SetNewTransportIdFromRequest(request, transportId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				RTC::PlainRtpTransport::Options options;

				if (request->data[JsonStringRemoteIp].isString())
					options.remoteIp = request->data[JsonStringRemoteIp].asString();

				if (request->data[JsonStringRemotePort].isUInt())
					options.remotePort = request->data[JsonStringRemotePort].asUInt();

				if (request->data[JsonStringLocalIp].isString())
					options.localIp = request->data[JsonStringLocalIp].asString();

				if (request->data[JsonStringPreferIPv4].isBool())
					options.preferIPv4 = request->data[JsonStringPreferIPv4].asBool();

				if (request->data[JsonStringPreferIPv6].isBool())
					options.preferIPv6 = request->data[JsonStringPreferIPv6].asBool();

				RTC::PlainRtpTransport* plainRtpTransport;

				try
				{
					// NOTE: This may throw.
					plainRtpTransport = new RTC::PlainRtpTransport(this, transportId, options);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// Insert into the map.
				this->transports[transportId] = plainRtpTransport;

				MS_DEBUG_DEV("PlainRtpTransport created [transportId:%" PRIu32 "]", transportId);

				auto data = plainRtpTransport->ToJson();

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

				std::string producerId;

				try
				{
					SetNewProducerIdFromRequest(request, producerId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

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

				if (!request->data[JsonStringKind].isString())
				{
					request->Reject("missing data.kind");

					return;
				}
				if (!request->data[JsonStringRtpParameters].isObject())
				{
					request->Reject("missing data.rtpParameters");

					return;
				}
				if (!request->data[JsonStringRtpMapping].isObject())
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

					if (kind == RTC::Media::Kind::ALL)
						MS_THROW_ERROR("invalid empty kind");

					// NOTE: This may throw.
					rtpParameters = RTC::RtpParameters(request->data[JsonStringRtpParameters]);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

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

						auto originPayloadType = static_cast<uint8_t>(pair[0].asUInt());
						auto mappedPayloadType = static_cast<uint8_t>(pair[1].asUInt());

						rtpMapping.codecPayloadTypes[originPayloadType] = mappedPayloadType;
					}

					for (auto& pair : jsonRtpMapping[JsonStringHeaderExtensionIds])
					{
						if (!pair.isArray() || pair.size() != 2 || !pair[0].isUInt() || !pair[1].isUInt())
							MS_THROW_ERROR("wrong rtpMapping entry");

						auto originHeaderExtensionId = static_cast<uint8_t>(pair[0].asUInt());
						auto mappedHeaderExtensionId = static_cast<uint8_t>(pair[1].asUInt());

						rtpMapping.headerExtensionIds[originHeaderExtensionId] = mappedHeaderExtensionId;
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
				auto* producer =
				  new RTC::Producer(producerId, kind, transport, rtpParameters, rtpMapping, paused);

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

				std::string consumerId;

				try
				{
					SetNewConsumerIdFromRequest(request, consumerId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

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

				if (!request->data[JsonStringKind].isString())
				{
					request->Reject("missing data.kind");

					return;
				}

				RTC::Media::Kind kind;
				std::string kindStr = request->data[JsonStringKind].asString();

				try
				{
					// NOTE: This may throw.
					kind = RTC::Media::GetKind(kindStr);

					if (kind == RTC::Media::Kind::ALL)
						MS_THROW_ERROR("invalid empty kind");
					else if (kind != producer->kind)
						MS_THROW_ERROR("not matching kind");
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				auto* consumer = new RTC::Consumer(consumerId, kind, producer->producerId);

				// If the Producer is paused tell it to the new Consumer.
				if (producer->IsPaused())
					consumer->ProducerPaused();

				// Add us as listener.
				consumer->AddListener(this);

				auto activeProfiles = producer->GetActiveProfiles();
				std::map<RTC::RtpEncodingParameters::Profile, const RTC::RtpStream*>::reverse_iterator it;

				for (it = activeProfiles.rbegin(); it != activeProfiles.rend(); ++it)
				{
					auto profile = it->first;
					auto stats   = it->second;

					consumer->AddProfile(profile, stats);
				}

				// Insert into the maps.
				this->consumers[consumerId] = consumer;
				this->mapProducerConsumers[producer].insert(consumer);
				this->mapConsumerProducer[consumer] = producer;

				MS_DEBUG_DEV("Consumer created [consumerId:%" PRIu32 "]", consumerId);

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

				delete transport;

				MS_DEBUG_DEV("Transport closed [transportId:%" PRIu32 "]", transport->transportId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_DUMP:
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

				auto json = transport->ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_GET_STATS:
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

				auto json = transport->GetStats();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_SET_REMOTE_DTLS_PARAMETERS:
			{
				static const Json::StaticString JsonStringRole{ "role" };
				static const Json::StaticString JsonStringClient{ "client" };
				static const Json::StaticString JsonStringServer{ "server" };
				static const Json::StaticString JsonStringFingerprints{ "fingerprints" };
				static const Json::StaticString JsonStringAlgorithm{ "algorithm" };
				static const Json::StaticString JsonStringValue{ "value" };

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

				if (!request->data[JsonStringFingerprints].isArray())
				{
					request->Reject("missing data.fingerprints");

					return;
				}

				RTC::DtlsTransport::Fingerprint remoteFingerprint;
				// Default value if missing.
				RTC::DtlsTransport::Role remoteRole{ RTC::DtlsTransport::Role::AUTO };

				auto& jsonArray = request->data[JsonStringFingerprints];

				for (Json::Value::ArrayIndex i = jsonArray.size() - 1; static_cast<int32_t>(i) >= 0; --i)
				{
					auto& jsonFingerprint = jsonArray[i];

					if (!jsonFingerprint.isObject())
					{
						request->Reject("wrong fingerprint");

						return;
					}
					if (!jsonFingerprint[JsonStringAlgorithm].isString() || !jsonFingerprint[JsonStringValue].isString())
					{
						request->Reject("missing data.fingerprint.algorithm and/or data.fingerprint.value");

						return;
					}

					remoteFingerprint.algorithm = RTC::DtlsTransport::GetFingerprintAlgorithm(
					  jsonFingerprint[JsonStringAlgorithm].asString());

					if (remoteFingerprint.algorithm != RTC::DtlsTransport::FingerprintAlgorithm::NONE)
					{
						remoteFingerprint.value = jsonFingerprint[JsonStringValue].asString();

						break;
					}
				}

				if (request->data[JsonStringRole].isString())
					remoteRole = RTC::DtlsTransport::StringToRole(request->data[JsonStringRole].asString());

				RTC::DtlsTransport::Role localRole;

				try
				{
					auto* webrtcTransport = dynamic_cast<RTC::WebRtcTransport*>(transport);

					// This may throw.
					localRole = webrtcTransport->SetRemoteDtlsParameters(remoteFingerprint, remoteRole);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				Json::Value data(Json::objectValue);

				switch (localRole)
				{
					case RTC::DtlsTransport::Role::CLIENT:
					{
						data[JsonStringRole] = JsonStringClient;

						break;
					}
					case RTC::DtlsTransport::Role::SERVER:
					{
						data[JsonStringRole] = JsonStringServer;

						break;
					}
					default:
					{
						MS_ABORT("invalid local DTLS role");
					}
				}

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_SET_REMOTE_PARAMETERS:
			{
				static const Json::StaticString JsonStringIp{ "ip" };
				static const Json::StaticString JsonStringPort{ "port" };

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

				if (!request->data[JsonStringIp].isString())
				{
					request->Reject("missing data.ip");

					return;
				}

				if (!request->data[JsonStringPort].isUInt())
				{
					request->Reject("missing data.port");

					return;
				}

				auto ip   = std::string{ request->data[JsonStringIp].asString() };
				auto port = uint32_t{ request->data[JsonStringPort].asUInt() };

				try
				{
					auto* plainRtpTransport = dynamic_cast<RTC::PlainRtpTransport*>(transport);

					// This may throw.
					plainRtpTransport->SetRemoteParameters(ip, port);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_SET_MAX_BITRATE:
			{
				static const Json::StaticString JsonStringBitrate{ "bitrate" };

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

				if (!request->data[JsonStringBitrate].isUInt())
				{
					request->Reject("missing data.bitrate");

					return;
				}

				auto bitrate = uint32_t{ request->data[JsonStringBitrate].asUInt() };

				auto* webrtcTransport = dynamic_cast<RTC::WebRtcTransport*>(transport);

				webrtcTransport->SetMaxBitrate(bitrate);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::TRANSPORT_CHANGE_UFRAG_PWD:
			{
				static const Json::StaticString JsonStringUsernameFragment{ "usernameFragment" };
				static const Json::StaticString JsonStringPassword{ "password" };

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

				std::string usernameFragment = Utils::Crypto::GetRandomString(16);
				std::string password         = Utils::Crypto::GetRandomString(32);

				auto* webrtcTransport = dynamic_cast<RTC::WebRtcTransport*>(transport);

				webrtcTransport->ChangeUfragPwd(usernameFragment, password);

				Json::Value data(Json::objectValue);

				data[JsonStringUsernameFragment] = usernameFragment;
				data[JsonStringPassword]         = password;

				request->Accept(data);

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

				delete producer;

				MS_DEBUG_DEV("Producer closed [producerId:%" PRIu32 "]", producer->producerId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_DUMP:
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

				auto json = producer->ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::PRODUCER_GET_STATS:
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

				auto json = producer->GetStats();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::PRODUCER_PAUSE:
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

				producer->Pause();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PRODUCER_RESUME:
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

				producer->Resume();

				request->Accept();

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

				delete consumer;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_ENABLE:
			{
				static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };

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

				if (!request->data[JsonStringRtpParameters].isObject())
				{
					request->Reject("missing data.rtpParameters");

					return;
				}

				RTC::RtpParameters rtpParameters;

				try
				{
					// NOTE: This may throw.
					rtpParameters = RTC::RtpParameters(request->data[JsonStringRtpParameters]);

					// NOTE: This may throw.
					consumer->Enable(transport, rtpParameters);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// Tell the Transport to handle the new Consumer.
				transport->HandleConsumer(consumer);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_DUMP:
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

				auto json = consumer->ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::CONSUMER_GET_STATS:
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

				auto json = consumer->GetStats();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::CONSUMER_PAUSE:
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

				consumer->Pause();

				request->Accept();

				break;
			}

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

				consumer->Resume();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_SET_PREFERRED_PROFILE:
			{
				static const Json::StaticString JsonStringProfile{ "profile" };

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

				if (!request->data[JsonStringProfile].isString())
				{
					request->Reject("missing data.profile");

					return;
				}

				std::string profileStr = request->data[JsonStringProfile].asString();
				auto it                = RTC::RtpEncodingParameters::string2Profile.find(profileStr);

				if (it == RTC::RtpEncodingParameters::string2Profile.end())
				{
					request->Reject("unknown profile");

					return;
				}

				auto profile = it->second;

				if (profile == RTC::RtpEncodingParameters::Profile::NONE)
				{
					request->Reject("invalid profile");

					return;
				}

				consumer->SetPreferredProfile(profile);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_SET_ENCODING_PREFERENCES:
			{
				static const Json::StaticString JsonStringQualityLayer{ "qualityLayer" };
				static const Json::StaticString JsonStringSpatialLayer{ "spatialLayer" };
				static const Json::StaticString JsonStringTemporalLayer{ "temporalLayer" };

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

				RTC::Codecs::EncodingContext::Preferences preferences;

				if (request->data[JsonStringQualityLayer].isUInt())
					preferences.qualityLayer = request->data[JsonStringQualityLayer].asUInt();
				if (request->data[JsonStringSpatialLayer].isUInt())
					preferences.spatialLayer = request->data[JsonStringSpatialLayer].asUInt();
				if (request->data[JsonStringTemporalLayer].isUInt())
					preferences.temporalLayer = request->data[JsonStringTemporalLayer].asUInt();

				consumer->SetEncodingPreferences(preferences);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::CONSUMER_REQUEST_KEY_FRAME:
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

				consumer->RequestKeyFrame();

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

	void Router::SetNewTransportIdFromRequest(Channel::Request* request, std::string& transportId) const
	{
		MS_TRACE();

		auto jsonTransportIdIt = request->internal.find("transportId");

		if (jsonTransportIdIt == request->internal.end() || !jsonTransportIdIt->is_string())
			MS_THROW_ERROR("request has no internal.transportId");

		transportId.assign(jsonTransportIdIt->get<std::string>());

		if (this->transports.find(transportId) != this->transports.end())
			MS_THROW_ERROR("a Transport with same transportId already exists");
	}

	RTC::Transport* Router::GetTransportFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTransportId{ "transportId" };

		auto jsonTransportId = request->internal[JsonStringTransportId];

		if (!jsonTransportId.isUInt())
			MS_THROW_ERROR("request has no internal.transportId");

		uint32_t transportId = jsonTransportId.asUInt();

		auto it = this->transports.find(transportId);
		if (it == this->transports.end())
			MS_THROW_ERROR("Transport not found");

		auto* transport = it->second;

		return transport;
	}

	void Router::SetNewProducerIdFromRequest(Channel::Request* request, std::string& producerId) const
	{
		MS_TRACE();

		auto jsonProducerIdIt = request->internal.find("producerId");

		if (jsonProducerIdIt == request->internal.end() || !jsonProducerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.producerId");

		producerId.assign(jsonProducerIdIt->get<std::string>());

		if (this->producers.find(producerId) != this->producers.end())
			MS_THROW_ERROR("a Producer with same producerId already exists");
	}

	RTC::Producer* Router::GetProducerFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringProducerId{ "producerId" };

		auto jsonProducerId = request->internal[JsonStringProducerId];

		if (!jsonProducerId.isUInt())
			MS_THROW_ERROR("request has no internal.producerId");

		uint32_t producerId = jsonProducerId.asUInt();

		auto it = this->producers.find(producerId);
		if (it == this->producers.end())
			MS_THROW_ERROR("Producer not found");

		auto* producer = it->second;

		return producer;
	}

	void Router::SetNewConsumerIdFromRequest(Channel::Request* request, std::string& consumerId) const
	{
		MS_TRACE();

		auto jsonConsumerIdIt = request->internal.find("consumerId");

		if (jsonConsumerIdIt == request->internal.end() || !jsonConsumerIdIt->is_string())
			MS_THROW_ERROR("request has no internal.consumerId");

		consumerId.assign(jsonConsumerIdIt->get<std::string>());

		if (this->consumers.find(consumerId) != this->consumers.end())
			MS_THROW_ERROR("a Consumer with same consumerId already exists");
	}

	RTC::Consumer* Router::GetConsumerFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringConsumerId{ "consumerId" };

		auto jsonConsumerId = request->internal[JsonStringConsumerId];

		if (!jsonConsumerId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.consumerId");

		uint32_t consumerId = jsonConsumerId.asUInt();

		auto it = this->consumers.find(consumerId);
		if (it == this->consumers.end())
			MS_THROW_ERROR("Consumer not found");

		auto* consumer = it->second;

		return consumer;
	}

	void Router::OnTransportClosed(RTC::Transport* transport)
	{
		MS_TRACE();

		this->transports.erase(transport->transportId);
	}

	void Router::OnProducerClosed(RTC::Producer* producer)
	{
		MS_TRACE();

		this->producers.erase(producer->producerId);

		// Remove the Producer from the map.
		// NOTE: It may not exist if it failed before being inserted into the maps.
		if (this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end())
		{
			// Iterate the map and close all Consumers associated to it.
			auto& consumers = this->mapProducerConsumers[producer];

			for (auto it = consumers.begin(); it != consumers.end();)
			{
				auto* consumer = *it;

				it = consumers.erase(it);
				delete consumer;
			}

			// Finally delete the Producer entry in the map.
			this->mapProducerConsumers.erase(producer);
		}
	}

	void Router::OnProducerPaused(RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto* consumer : consumers)
		{
			consumer->ProducerPaused();
		}
	}

	void Router::OnProducerResumed(RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto* consumer : consumers)
		{
			consumer->ProducerResumed();
		}
	}

	void Router::OnProducerRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers[producer];

		// Send the packet to all the consumers associated to the producer.
		for (auto* consumer : consumers)
		{
			consumer->SendRtpPacket(packet);
		}
	}

	void Router::OnProducerStreamEnabled(
	  RTC::Producer* producer, const RTC::RtpStream* rtpStream, uint32_t translatedSsrc)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto* consumer : consumers)
		{
			consumer->AddStream(rtpStream, translatedSsrc);
		}
	}

	void Router::OnProducerProfileDisabled(RTC::Producer* producer, const RTC::RtpStream* rtpStream)
	{
		MS_TRACE();

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto* consumer : consumers)
		{
			consumer->RemoveStream(rtpStream, translatedSsrc);
		}
	}

	void Router::OnConsumerClosed(RTC::Consumer* consumer)
	{
		MS_TRACE();

		this->consumers.erase(consumer->consumerId);

		for (auto& kv : this->mapProducerConsumers)
		{
			auto& consumers  = kv.second;
			size_t numErased = consumers.erase(consumer);

			// If we have really deleted a Consumer then we are done since we know
			// that it just belongs to a single Producer.
			if (numErased > 0)
				break;
		}

		// Finally delete the Consumer entry in the map.
		this->mapConsumerProducer.erase(consumer);
	}

	void Router::OnConsumerKeyFrameRequired(RTC::Consumer* consumer)
	{
		MS_TRACE();

		auto* producer = this->mapConsumerProducer[consumer];

		producer->RequestKeyFrame();
	}
} // namespace RTC
