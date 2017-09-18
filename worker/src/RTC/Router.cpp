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
			auto* producer  = kv.first;
			auto& consumers = kv.second;
			Json::Value jsonProducers(Json::arrayValue);

			for (auto* consumer : consumers)
			{
				jsonProducers.append(std::to_string(consumer->consumerId));
			}

			jsonMapProducerConsumers[std::to_string(producer->producerId)] = jsonProducers;
		}
		json[JsonStringMapProducerConsumers] = jsonMapProducerConsumers;

		// Add mapConsumerProducer.
		for (auto& kv : this->mapConsumerProducer)
		{
			auto* consumer = kv.first;
			auto* producer = kv.second;

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
				static const Json::StaticString JsonStringUdp{ "udp" };
				static const Json::StaticString JsonStringTcp{ "tcp" };
				static const Json::StaticString JsonStringPreferIPv4{ "preferIPv4" };
				static const Json::StaticString JsonStringPreferIPv6{ "preferIPv6" };
				static const Json::StaticString JsonStringPreferUdp{ "preferUdp" };
				static const Json::StaticString JsonStringPreferTcp{ "preferTcp" };

				uint32_t transportId;

				try
				{
					transportId = GetNewTransportIdFromRequest(request);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				RTC::Transport::TransportOptions options;

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

				RTC::Transport* transport;

				try
				{
					// NOTE: This may throw.
					transport = new RTC::Transport(this, this->notifier, transportId, options);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// Insert into the map.
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

				uint32_t producerId;

				try
				{
					producerId = GetNewProducerIdFromRequest(request);
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
				auto* producer = new RTC::Producer(
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

				uint32_t consumerId;

				try
				{
					consumerId = GetNewConsumerIdFromRequest(request);
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

				auto* consumer = new RTC::Consumer(this->notifier, consumerId, kind, producer->producerId);

				// If the Producer is paused tell it to the new Consumer.
				if (producer->IsPaused())
					consumer->SourcePause();

				// Add us as listener.
				consumer->AddListener(this);

				auto profiles = producer->GetHealthyProfiles();
				std::set<RtpEncodingParameters::Profile>::reverse_iterator it;

				for (it = profiles.rbegin(); it != profiles.rend(); ++it)
				{
					auto profile = *it;

					consumer->AddProfile(profile);
				}

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
				{
					MS_DEBUG_DEV("audiolevels event enabled");

					this->audioLevelsTimer->Start(AudioLevelsInterval, AudioLevelsInterval);
				}
				else
				{
					MS_DEBUG_DEV("audiolevels event disabled");

					this->audioLevelsTimer->Stop();
				}

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

				transport->Destroy();

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
					// This may throw.
					localRole = transport->SetRemoteDtlsParameters(remoteFingerprint, remoteRole);
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

				transport->SetMaxBitrate(bitrate);

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

				transport->ChangeUfragPwd(usernameFragment, password);

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

				producer->Destroy();

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

			case Channel::Request::MethodId::PRODUCER_UPDATE_RTP_PARAMETERS:
			{
				static const Json::StaticString JsonStringRtpParameters{ "rtpParameters" };

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
					producer->UpdateRtpParameters(rtpParameters);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				request->Accept();

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

				consumer->Destroy();

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

				if (profile == RTC::RtpEncodingParameters::Profile::NONE || profile == RTC::RtpEncodingParameters::Profile::DEFAULT)
				{
					request->Reject("invalid profile");

					return;
				}

				consumer->SetPreferredProfile(profile);

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

	/**
	 * Looks for internal.transportId numeric field and ensures no Transport exists
	 * with such a id.
	 * NOTE: It may throw if invalid request or Transport already exists.
	 */
	uint32_t Router::GetNewTransportIdFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTransportId{ "transportId" };

		auto jsonTransportId = request->internal[JsonStringTransportId];

		if (!jsonTransportId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.transportId");

		uint32_t transportId = jsonTransportId.asUInt();

		if (this->transports.find(transportId) != this->transports.end())
			MS_THROW_ERROR("a Transport with same transportId already exists");

		return transportId;
	}

	/**
	 * Looks for internal.transportId numeric field and returns the corresponding
	 * Transport.
	 * NOTE: It may throw if invalid request or Transport does not exist.
	 */
	RTC::Transport* Router::GetTransportFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTransportId{ "transportId" };

		auto jsonTransportId = request->internal[JsonStringTransportId];

		if (!jsonTransportId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.transportId");

		uint32_t transportId = jsonTransportId.asUInt();

		auto it = this->transports.find(transportId);
		if (it == this->transports.end())
			MS_THROW_ERROR("Transport not found");

		auto* transport = it->second;

		return transport;
	}

	/**
	 * Looks for internal.producerId numeric field and ensures no Producer exists
	 * with such a id.
	 * NOTE: It may throw if invalid request or Producer already exists.
	 */
	uint32_t Router::GetNewProducerIdFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringProducerId{ "producerId" };

		auto jsonProducerId = request->internal[JsonStringProducerId];

		if (!jsonProducerId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.producerId");

		uint32_t producerId = jsonProducerId.asUInt();

		if (this->producers.find(producerId) != this->producers.end())
			MS_THROW_ERROR("a Producer with same producerId already exists");

		return producerId;
	}

	/**
	 * Looks for internal.producerId numeric field and returns the corresponding
	 * Producer.
	 * NOTE: It may throw if invalid request or Producer does not exist.
	 */
	RTC::Producer* Router::GetProducerFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringProducerId{ "producerId" };

		auto jsonProducerId = request->internal[JsonStringProducerId];

		if (!jsonProducerId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.producerId");

		uint32_t producerId = jsonProducerId.asUInt();

		auto it = this->producers.find(producerId);
		if (it == this->producers.end())
			MS_THROW_ERROR("Producer not found");

		auto* producer = it->second;

		return producer;
	}

	/**
	 * Looks for internal.consumerId numeric field and ensures no Consumer exists
	 * with such a id.
	 * NOTE: It may throw if invalid request or Consumer already exists.
	 */
	uint32_t Router::GetNewConsumerIdFromRequest(Channel::Request* request) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringConsumerId{ "consumerId" };

		auto jsonConsumerId = request->internal[JsonStringConsumerId];

		if (!jsonConsumerId.isUInt())
			MS_THROW_ERROR("Request has not numeric internal.consumerId");

		uint32_t consumerId = jsonConsumerId.asUInt();

		if (this->consumers.find(consumerId) != this->consumers.end())
			MS_THROW_ERROR("a Consumer with same consumerId already exists");

		return consumerId;
	}

	/**
	 * Looks for internal.consumerId numeric field and returns the corresponding
	 * Consumer.
	 * NOTE: It may throw if invalid request or Consumer does not exist.
	 */
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

	void Router::OnTransportReceiveRtcpFeedback(
	  RTC::Transport* /*transport*/, RTC::Consumer* consumer, RTC::RTCP::FeedbackPsPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapConsumerProducer.find(consumer) != this->mapConsumerProducer.end(),
		  "Consumer not present in mapConsumerProducer");

		auto* producer = this->mapConsumerProducer[consumer];

		producer->ReceiveRtcpFeedback(packet);
	}

	void Router::OnProducerClosed(RTC::Producer* producer)
	{
		MS_TRACE();

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

	void Router::OnProducerHasRemb(RTC::Producer* /*producer*/)
	{
		// Do nothing.
	}

	void Router::OnProducerRtpParametersUpdated(RTC::Producer* producer)
	{
		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto* consumer : consumers)
		{
			consumer->SourceRtpParametersUpdated();
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
			consumer->SourcePause();
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
			if (consumer->IsEnabled())
				consumer->SourceResume();
		}
	}

	void Router::OnProducerRtpPacket(
	  RTC::Producer* producer, RTC::RtpPacket* packet, RTC::RtpEncodingParameters::Profile profile)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		// Send the RtpPacket to all the Consumers associated to the Producer
		// from which it was received.
		for (auto* consumer : consumers)
		{
			if (consumer->IsEnabled())
				consumer->SendRtpPacket(packet, profile);
		}

		// Update audio levels.
		if (this->audioLevelsEventEnabled && producer->kind == RTC::Media::Kind::AUDIO)
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

	void Router::OnProducerProfileEnabled(
	  RTC::Producer* producer, RTC::RtpEncodingParameters::Profile profile)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto* consumer : consumers)
		{
			consumer->AddProfile(profile);
		}
	}

	void Router::OnProducerProfileDisabled(
	  RTC::Producer* producer, RTC::RtpEncodingParameters::Profile profile)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		  "Producer not present in mapProducerConsumers");

		auto& consumers = this->mapProducerConsumers[producer];

		for (auto* consumer : consumers)
		{
			consumer->RemoveProfile(profile);
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

	void Router::OnConsumerKeyFrameRequired(RTC::Consumer* consumer)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->mapConsumerProducer.find(consumer) != this->mapConsumerProducer.end(),
		  "Consumer not present in mapConsumerProducer");

		auto* producer = this->mapConsumerProducer[consumer];

		producer->RequestKeyFrame();
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
				auto* producer            = kv.first;
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
