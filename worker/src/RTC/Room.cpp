#define MS_CLASS "RTC::Room"
// #define MS_LOG_DEV

#include "RTC/Room.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Utils.hpp"
#include <cmath> // std::lround()
#include <set>
#include <string>
#include <vector>

namespace RTC
{
	/* Static. */

	static constexpr uint64_t AudioLevelsInterval{ 500 }; // In ms.

	/* Instance methods. */

	Room::Room(Listener* listener, Channel::Notifier* notifier, uint32_t roomId)
	    : roomId(roomId), listener(listener), notifier(notifier)
	{
		MS_TRACE();

		// Set the audio levels timer.
		this->audioLevelsTimer = new Timer(this);
	}

	Room::~Room()
	{
		MS_TRACE();
	}

	void Room::Destroy()
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };

		Json::Value eventData(Json::objectValue);

		// Close all the Peers.
		// NOTE: Upon Peer closure the onPeerClosed() method is called which
		// removes it from the map, so this is the safe way to iterate the map
		// and remove elements.
		for (auto it = this->peers.begin(); it != this->peers.end();)
		{
			auto* peer = it->second;

			it = this->peers.erase(it);
			peer->Destroy();
		}

		// Close the audio level timer.
		this->audioLevelsTimer->Destroy();

		// Notify.
		eventData[JsonStringClass] = "Room";
		this->notifier->Emit(this->roomId, "close", eventData);

		// Notify the listener.
		this->listener->OnRoomClosed(this);

		delete this;
	}

	Json::Value Room::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringRoomId{ "roomId" };
		static const Json::StaticString JsonStringPeers{ "peers" };
		static const Json::StaticString JsonStringMapProducerConsumers{ "mapProducerConsumers" };
		static const Json::StaticString JsonStringMapConsumerProducer{ "mapConsumerProducer" };
		static const Json::StaticString JsonStringAudioLevelsEventEnabled{ "audioLevelsEventEnabled" };

		Json::Value json(Json::objectValue);
		Json::Value jsonPeers(Json::arrayValue);
		Json::Value jsonMapProducerConsumers(Json::objectValue);
		Json::Value jsonMapConsumerProducer(Json::objectValue);

		// Add `roomId`.
		json[JsonStringRoomId] = Json::UInt{ this->roomId };

		// Add `peers`.
		for (auto& kv : this->peers)
		{
			auto* peer = kv.second;

			jsonPeers.append(peer->ToJson());
		}
		json[JsonStringPeers] = jsonPeers;

		// Add `mapProducerConsumers`.
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

		// Add `mapConsumerProducer`.
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

	void Room::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::ROOM_CLOSE:
			{
#ifdef MS_LOG_DEV
				uint32_t roomId = this->roomId;
#endif

				Destroy();

				MS_DEBUG_DEV("Room closed [roomId:%" PRIu32 "]", roomId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::ROOM_DUMP:
			{
				auto json = ToJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::ROOM_CREATE_PEER:
			{
				static const Json::StaticString JsonStringPeerName{ "peerName" };

				RTC::Peer* peer;
				uint32_t peerId;
				std::string peerName;

				try
				{
					peer = GetPeerFromRequest(request, &peerId);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				if (peer != nullptr)
				{
					request->Reject("Peer already exists");

					return;
				}

				if (!request->internal[JsonStringPeerName].isString())
				{
					request->Reject("Request has not string internal.peerName");

					return;
				}

				peerName = request->internal[JsonStringPeerName].asString();

				try
				{
					peer = new RTC::Peer(this, this->notifier, peerId, peerName);
				}
				catch (const MediaSoupError& error)
				{
					request->Reject(error.what());

					return;
				}

				// Store the new Peer.
				this->peers[peerId] = peer;

				MS_DEBUG_DEV("Peer created [peerId:%u, peerName:'%s']", peerId, peerName.c_str());

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::ROOM_SET_AUDIO_LEVELS_EVENT:
			{
				static const Json::StaticString JsonStringEnabled{ "enabled" };

				if (!request->data[JsonStringEnabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				bool audioLevelsEventEnabled = request->data[JsonStringEnabled].asBool();

				if (audioLevelsEventEnabled == this->audioLevelsEventEnabled)
					return;

				// Clear map of audio levels.
				this->mapProducerAudioLevels.clear();

				// Start or stop audio levels periodic timer.
				if (audioLevelsEventEnabled)
					this->audioLevelsTimer->Start(AudioLevelsInterval, AudioLevelsInterval);
				else
					this->audioLevelsTimer->Stop();

				this->audioLevelsEventEnabled = audioLevelsEventEnabled;

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::PEER_CLOSE:
			case Channel::Request::MethodId::PEER_DUMP:
			case Channel::Request::MethodId::PEER_CREATE_TRANSPORT:
			case Channel::Request::MethodId::PEER_CREATE_PRODUCER:
			case Channel::Request::MethodId::TRANSPORT_CLOSE:
			case Channel::Request::MethodId::TRANSPORT_DUMP:
			case Channel::Request::MethodId::TRANSPORT_SET_REMOTE_DTLS_PARAMETERS:
			case Channel::Request::MethodId::TRANSPORT_SET_MAX_BITRATE:
			case Channel::Request::MethodId::TRANSPORT_CHANGE_UFRAG_PWD:
			case Channel::Request::MethodId::PRODUCER_CLOSE:
			case Channel::Request::MethodId::PRODUCER_DUMP:
			case Channel::Request::MethodId::PRODUCER_RECEIVE:
			case Channel::Request::MethodId::PRODUCER_SET_RTP_RAW_EVENT:
			case Channel::Request::MethodId::PRODUCER_SET_RTP_OBJECT_EVENT:
			case Channel::Request::MethodId::CONSUMER_DUMP:
			case Channel::Request::MethodId::CONSUMER_SET_TRANSPORT:
			case Channel::Request::MethodId::CONSUMER_DISABLE:
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

	RTC::Peer* Room::GetPeerFromRequest(Channel::Request* request, uint32_t* peerId) const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringPeerId{ "peerId" };

		auto jsonPeerId = request->internal[JsonStringPeerId];

		if (!jsonPeerId.isUInt())
			MS_THROW_ERROR("Request has not numeric .peerId field");

		// If given, fill peerId.
		if (peerId != nullptr)
			*peerId = jsonPeerId.asUInt();

		auto it = this->peers.find(jsonPeerId.asUInt());

		if (it != this->peers.end())
		{
			auto* peer = it->second;

			return peer;
		}

		return nullptr;
	}

	inline void Room::AddConsumerForProducer(RTC::Peer* consumerPeer, const RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(producer->GetParameters(), "Producer has no parameters");

		uint32_t consumerId = Utils::Crypto::GetRandomUInt(10000000, 99999999);
		auto consumer = new RTC::Consumer(consumerPeer, this->notifier, consumerId, producer->kind);

		// Store into the maps.
		this->mapProducerConsumers[producer].insert(consumer);
		this->mapConsumerProducer[consumer] = producer;

		auto rtpParameters        = producer->GetParameters();
		auto associatedProducerId = producer->producerId;

		// Attach the Consumer to the peer.
		consumerPeer->AddConsumer(consumer, rtpParameters, associatedProducerId);
	}

	void Room::OnPeerClosed(const RTC::Peer* peer)
	{
		MS_TRACE();

		this->peers.erase(peer->peerId);
	}

	// TODO: No. Instead let's producer.broadcast() or producer.connect(peer) in JS.
	void Room::OnPeerProducerRtpParameters(const RTC::Peer* peer, RTC::Producer* producer)
	{
		MS_TRACE();

		MS_ASSERT(producer->GetParameters(), "Producer has no parameters");

		// If this is a new Producer, iterate all the peers but this one and
		// create a Consumer associated to this Producer for each Peer.
		if (this->mapProducerConsumers.find(producer) == this->mapProducerConsumers.end())
		{
			// Ensure the entry will exist even with an empty array.
			this->mapProducerConsumers[producer];

			// for (auto& kv : this->peers)
			// {
			// 	auto* consumerPeer = kv.second;

			// 	// Skip producing Peer.
			// 	if (consumerPeer == peer)
			// 		continue;

			// 	AddConsumerForProducer(consumerPeer, producer);
			// }
		}
		// If this is not a new Producer let's retrieve its updated parameters
		// and update with them all the associated Consumers.
		else
		{
			// for (auto consumer : this->mapProducerConsumers[producer])
			// {
			// 	// Provide the Consumer with the parameters of the Producer.
			// 	consumer->Send(producer->GetParameters());
			// }
		}
	}

	void Room::OnPeerProducerClosed(const RTC::Peer* /*peer*/, const RTC::Producer* producer)
	{
		MS_TRACE();

		// If the Producer is in the map, iterate the map and close all the
		// Consumers associated to the closed Producer.
		if (this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end())
		{
			// Make a copy of the set of Consumers given that Destroy() will be called
			// in all of them, producing onPeerConsumerClosed() that will remove it
			// from the map.
			auto consumers = this->mapProducerConsumers[producer];

			// Safely iterate the copy of the set.
			for (auto& consumer : consumers)
			{
				consumer->Destroy();
			}

			// Finally delete the Producer entry in the map.
			this->mapProducerConsumers.erase(producer);
		}
	}

	void Room::OnPeerConsumerClosed(const RTC::Peer* /*peer*/, RTC::Consumer* consumer)
	{
		MS_TRACE();

		for (auto& kv : this->mapProducerConsumers)
		{
			auto& consumers = kv.second;

			consumers.erase(consumer);
		}

		this->mapConsumerProducer.erase(consumer);
	}

	void Room::OnPeerRtpPacket(const RTC::Peer* /*peer*/, RTC::Producer* producer, RTC::RtpPacket* packet)
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

				this->mapProducerAudioLevels[producer].push_back(dBov);
			}
		}
	}

	void Room::OnPeerRtcpReceiverReport(
	    const RTC::Peer* /*peer*/, RTC::Consumer* consumer, RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapConsumerProducer.find(consumer) != this->mapConsumerProducer.end(),
		    "Consumer not present in the map");

		consumer->ReceiveRtcpReceiverReport(report);
	}

	void Room::OnPeerRtcpFeedback(
	    const RTC::Peer* /*peer*/, RTC::Consumer* consumer, RTC::RTCP::FeedbackPsPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapConsumerProducer.find(consumer) != this->mapConsumerProducer.end(),
		    "Consumer not present in the map");

		auto& producer = this->mapConsumerProducer[consumer];

		producer->ReceiveRtcpFeedback(packet);
	}

	void Room::OnPeerRtcpFeedback(
	    const RTC::Peer* /*peer*/, RTC::Consumer* consumer, RTC::RTCP::FeedbackRtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapConsumerProducer.find(consumer) != this->mapConsumerProducer.end(),
		    "Consumer not present in the map");

		auto& producer = this->mapConsumerProducer[consumer];

		producer->ReceiveRtcpFeedback(packet);
	}

	void Room::OnPeerRtcpSenderReport(
	    const RTC::Peer* /*peer*/, RTC::Producer* producer, RTC::RTCP::SenderReport* report)
	{
		MS_TRACE();

		// Producer needs the sender report in order to generate it's receiver report.
		producer->ReceiveRtcpSenderReport(report);

		MS_ASSERT(
		    this->mapProducerConsumers.find(producer) != this->mapProducerConsumers.end(),
		    "Producer not present in the map");
	}

	void Room::OnFullFrameRequired(RTC::Peer* /*peer*/, RTC::Consumer* consumer)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->mapConsumerProducer.find(consumer) != this->mapConsumerProducer.end(),
		    "Consumer not present in the map");

		auto& producer = this->mapConsumerProducer[consumer];

		producer->RequestFullFrame();
	}

	inline void Room::OnTimer(Timer* timer)
	{
		MS_TRACE();

		static const Json::StaticString JsonStringClass{ "class" };
		static const Json::StaticString JsonStringEntries{ "entries" };

		// Audio levels timer.
		if (timer == this->audioLevelsTimer)
		{
			std::unordered_map<RTC::Producer*, int8_t> mapProducerAudioLevel;

			for (auto& kv : this->mapProducerAudioLevels)
			{
				auto producer = kv.first;
				auto& dBovs   = kv.second;
				int8_t avgdBov{ -127 };

				if (!dBovs.empty())
				{
					int16_t sumdBovs{ 0 };

					for (auto& dBov : dBovs)
					{
						sumdBovs += dBov;
					}

					avgdBov = static_cast<int8_t>(std::lround(sumdBovs / static_cast<int16_t>(dBovs.size())));
				}

				mapProducerAudioLevel[producer] = avgdBov;
			}

			// Clear map.
			this->mapProducerAudioLevels.clear();

			// Emit event.
			Json::Value eventData(Json::objectValue);

			eventData[JsonStringClass]   = "Room";
			eventData[JsonStringEntries] = Json::arrayValue;

			for (auto& kv : mapProducerAudioLevel)
			{
				auto& producerId = kv.first->producerId;
				auto& audioLevel = kv.second;
				Json::Value entry(Json::arrayValue);

				entry.append(Json::UInt{ producerId });
				entry.append(Json::Int{ audioLevel });

				eventData[JsonStringEntries].append(entry);
			}

			this->notifier->Emit(this->roomId, "audiolevels", eventData);
		}
	}
} // namespace RTC
