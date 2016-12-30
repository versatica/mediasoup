#define MS_CLASS "RTC::Room"

#include "RTC/Room.h"
#include "RTC/RTCP/FeedbackRtpNack.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include "Utils.h"
#include <string>
#include <vector>
#include <set>

namespace RTC
{
	/* Class variables. */

	RTC::RtpCapabilities Room::supportedCapabilities;

	/* Class methods. */

	void Room::ClassInit()
	{
		MS_TRACE();

		// Parse all RTP capabilities.
		{
			Json::CharReaderBuilder builder;
			Json::Value settings = Json::nullValue;

			builder.strictMode(&settings);

			Json::CharReader* jsonReader = builder.newCharReader();

			// NOTE: This line is auto-generated from data/supportedCapabilities.js.
			const std::string capabilities = R"({"headerExtensions":[{"kind":"","uri":"urn:ietf:params:rtp-hdrext:sdes:mid","preferredId":1,"preferredEncrypt":false}],"fecMechanisms":[]})";

			Json::Value json;
			std::string json_parse_error;

			if (!jsonReader->parse(capabilities.c_str(), capabilities.c_str() + capabilities.length(), &json, &json_parse_error))
			{
				delete jsonReader;

				MS_THROW_ERROR_STD("JSON parsing error in supported RTP capabilities: %s", json_parse_error.c_str());
			}

			delete jsonReader;

			try
			{
				Room::supportedCapabilities = RTC::RtpCapabilities(json, RTC::Scope::ROOM_CAPABILITY);
			}
			catch (const MediaSoupError &error)
			{
				MS_THROW_ERROR_STD("wrong supported RTP capabilities: %s", error.what());
			}
		}
	}

	/* Instance methods. */

	Room::Room(Listener* listener, Channel::Notifier* notifier, uint32_t roomId, Json::Value& data) :
		roomId(roomId),
		listener(listener),
		notifier(notifier),
		rtpRetransmissionContainer(18) // Can retransmit up to 17 RTP packets.
	{
		MS_TRACE();

		static const Json::StaticString k_mediaCodecs("mediaCodecs");

		// `mediaCodecs` is optional.
		if (data[k_mediaCodecs].isArray())
		{
			auto& json_mediaCodecs = data[k_mediaCodecs];

			std::vector<RTC::RtpCodecParameters> mediaCodecs;

			for (Json::UInt i = 0; i < json_mediaCodecs.size(); i++)
			{
				RTC::RtpCodecParameters mediaCodec(json_mediaCodecs[i], RTC::Scope::ROOM_CAPABILITY);

				// Ignore feature codecs.
				if (mediaCodec.mime.IsFeatureCodec())
					continue;

				mediaCodecs.push_back(mediaCodec);
			}

			// Set room RTP capabilities.
			// NOTE: This may throw.
			SetCapabilities(mediaCodecs);
		}
	}

	Room::~Room()
	{
		MS_TRACE();
	}

	void Room::Close()
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");

		Json::Value event_data(Json::objectValue);

		// Close all the Peers.
		// NOTE: Upon Peer closure the onPeerClosed() method is called which
		// removes it from the map, so this is the safe way to iterate the map
		// and remove elements.
		for (auto it = this->peers.begin(); it != this->peers.end();)
		{
			RTC::Peer* peer = it->second;

			it = this->peers.erase(it);
			peer->Close();
		}

		// Notify.
		event_data[k_class] = "Room";
		this->notifier->Emit(this->roomId, "close", event_data);

		// Notify the listener.
		this->listener->onRoomClosed(this);

		delete this;
	}

	Json::Value Room::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_roomId("roomId");
		static const Json::StaticString k_capabilities("capabilities");
		static const Json::StaticString k_peers("peers");
		static const Json::StaticString k_mapRtpReceiverRtpSenders("mapRtpReceiverRtpSenders");
		static const Json::StaticString k_mapRtpSenderRtpReceiver("mapRtpSenderRtpReceiver");

		Json::Value json(Json::objectValue);
		Json::Value json_peers(Json::arrayValue);
		Json::Value json_mapRtpReceiverRtpSenders(Json::objectValue);
		Json::Value json_mapRtpSenderRtpReceiver(Json::objectValue);

		// Add `roomId`.
		json[k_roomId] = (Json::UInt)this->roomId;

		// Add `capabilities`.
		json[k_capabilities] = this->capabilities.toJson();

		// Add `peers`.
		for (auto& kv : this->peers)
		{
			RTC::Peer* peer = kv.second;

			json_peers.append(peer->toJson());
		}
		json[k_peers] = json_peers;

		// Add `mapRtpReceiverRtpSenders`.
		for (auto& kv : this->mapRtpReceiverRtpSenders)
		{
			auto rtpReceiver = kv.first;
			auto& rtpSenders = kv.second;
			Json::Value json_rtpReceivers(Json::arrayValue);

			for (auto& rtpSender : rtpSenders)
			{
				json_rtpReceivers.append(std::to_string(rtpSender->rtpSenderId));
			}

			json_mapRtpReceiverRtpSenders[std::to_string(rtpReceiver->rtpReceiverId)] = json_rtpReceivers;
		}
		json[k_mapRtpReceiverRtpSenders] = json_mapRtpReceiverRtpSenders;

		// Add `mapRtpSenderRtpReceiver`.
		for (auto& kv : this->mapRtpSenderRtpReceiver)
		{
			auto rtpSender = kv.first;
			auto rtpReceiver = kv.second;

			json_mapRtpSenderRtpReceiver[std::to_string(rtpSender->rtpSenderId)] = std::to_string(rtpReceiver->rtpReceiverId);
		}
		json[k_mapRtpSenderRtpReceiver] = json_mapRtpSenderRtpReceiver;

		return json;
	}

	void Room::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::room_close:
			{
				uint32_t roomId = this->roomId;

				Close();

				MS_DEBUG("Room closed [roomId:%" PRIu32 "]", roomId);
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::room_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::room_createPeer:
			{
				static const Json::StaticString k_peerName("peerName");

				RTC::Peer* peer;
				uint32_t peerId;
				std::string peerName;

				try
				{
					peer = GetPeerFromRequest(request, &peerId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (peer)
				{
					request->Reject("Peer already exists");
					return;
				}

				if (!request->internal[k_peerName].isString())
				{
					request->Reject("Request has not string internal.peerName");
					return;
				}

				peerName = request->internal[k_peerName].asString();

				try
				{
					peer = new RTC::Peer(this, this->notifier, peerId, peerName);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				// Store the new Peer.
				this->peers[peerId] = peer;

				MS_DEBUG("Peer created [peerId:%u, peerName:'%s']", peerId, peerName.c_str());

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::peer_close:
			case Channel::Request::MethodId::peer_dump:
			case Channel::Request::MethodId::peer_setCapabilities:
			case Channel::Request::MethodId::peer_createTransport:
			case Channel::Request::MethodId::peer_createRtpReceiver:
			case Channel::Request::MethodId::transport_close:
			case Channel::Request::MethodId::transport_dump:
			case Channel::Request::MethodId::transport_setRemoteDtlsParameters:
			case Channel::Request::MethodId::rtpReceiver_close:
			case Channel::Request::MethodId::rtpReceiver_dump:
			case Channel::Request::MethodId::rtpReceiver_receive:
			case Channel::Request::MethodId::rtpReceiver_setRtpRawEvent:
			case Channel::Request::MethodId::rtpReceiver_setRtpObjectEvent:
			case Channel::Request::MethodId::rtpSender_dump:
			case Channel::Request::MethodId::rtpSender_setTransport:
			{
				RTC::Peer* peer;

				try
				{
					peer = GetPeerFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!peer)
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

	RTC::Peer* Room::GetPeerFromRequest(Channel::Request* request, uint32_t* peerId)
	{
		MS_TRACE();

		static const Json::StaticString k_peerId("peerId");

		auto json_peerId = request->internal[k_peerId];

		if (!json_peerId.isUInt())
			MS_THROW_ERROR("Request has not numeric .peerId field");

		// If given, fill peerId.
		if (peerId)
			*peerId = json_peerId.asUInt();

		auto it = this->peers.find(json_peerId.asUInt());
		if (it != this->peers.end())
		{
			RTC::Peer* peer = it->second;

			return peer;
		}
		else
		{
			return nullptr;
		}
	}

	void Room::SetCapabilities(std::vector<RTC::RtpCodecParameters>& mediaCodecs)
	{
		MS_TRACE();

		// Set codecs.
		{
			// Available dynamic payload types.
			static const std::vector<uint8_t> dynamicPayloadTypes
			{
				100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
				110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
				120, 121, 122, 123, 124, 125, 126, 127,
				 96,  97,  98,  99,
				 77,  78,  79,  80,  81,  82,  83,  84,  85,  86,
				 87,  88,  89,  90,  91,  92,  93,  94,  95,
				 35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
				 45,  46,  47,  48,  49,  50,  51,  52,  53,  54,
				 55,  56,  57,  58,  59,  60,  61,  62,  63,  64,
				 65,  66,  67,  68,  69,  70,  71
			};
			// Iterator for available dynamic payload types.
			auto dynamicPayloadTypeIt = dynamicPayloadTypes.begin();
			// Payload types used by the room.
			std::set<uint8_t> roomPayloadTypes;
			// Given media kinds.
			std::set<RTC::Media::Kind> roomKinds;

			// Set the given room codecs.
			for (auto& mediaCodec : mediaCodecs)
			{
				// The room has this kind.
				roomKinds.insert(mediaCodec.kind);

				// Set unique PT.

				// If the codec has PT and it's not already used, let it untouched.
				if (mediaCodec.hasPayloadType &&
						roomPayloadTypes.find(mediaCodec.payloadType) == roomPayloadTypes.end())
				{
					;
				}
				// Otherwise assign an available PT.
				else
				{
					uint8_t payloadType;

					while (dynamicPayloadTypeIt != dynamicPayloadTypes.end())
					{
						payloadType = *dynamicPayloadTypeIt;
						dynamicPayloadTypeIt++;

						if (roomPayloadTypes.find(payloadType) == roomPayloadTypes.end())
						{
							// Assign PT.
							mediaCodec.payloadType = payloadType;
							mediaCodec.hasPayloadType = true;

							break;
						}
					}

					// If no one found, throw.
					if (!mediaCodec.hasPayloadType)
						MS_THROW_ERROR("no more available dynamic payload types for given media codecs");
				}

				// Store the selected PT.
				roomPayloadTypes.insert(mediaCodec.payloadType);

				// Append the codec to the room capabilities.
				this->capabilities.codecs.push_back(mediaCodec);
			}
		}

		// Add supported RTP header extensions.
		this->capabilities.headerExtensions = Room::supportedCapabilities.headerExtensions;

		// Add supported FEC mechanisms.
		this->capabilities.fecMechanisms = Room::supportedCapabilities.fecMechanisms;
	}

	void Room::onPeerClosed(RTC::Peer* peer)
	{
		MS_TRACE();

		this->peers.erase(peer->peerId);
	}

	void Room::onPeerCapabilities(RTC::Peer* peer, RTC::RtpCapabilities* capabilities)
	{
		MS_TRACE();

		// Remove those peer's capabilities not supported by the room.

		// Remove unsupported codecs.
		for (auto it = capabilities->codecs.begin(); it != capabilities->codecs.end();)
		{
			auto& peerCodecCapability = *it;
			auto it2 = this->capabilities.codecs.begin();

			for (; it2 != this->capabilities.codecs.end(); ++it2)
			{
				auto& roomCodecCapability = *it2;

				if (roomCodecCapability.Matches(peerCodecCapability))
					break;
			}

			if (it2 != this->capabilities.codecs.end())
			{
				it++;
			}
			else
			{
				it = capabilities->codecs.erase(it);
			}
		}

		// TODO: Remove unsupported header extensions.

		// TODO: Remove unsupported fec mechanisms.

		// Get all the ready RtpReceivers of the others Peers in the Room and
		// create RtpSenders for this new Peer.
		for (auto& kv : this->peers)
		{
			RTC::Peer* receiver_peer = kv.second;

			for (auto rtpReceiver : receiver_peer->GetRtpReceivers())
			{
				// Skip if the RtpReceiver has not parameters.
				if (!rtpReceiver->GetParameters())
					continue;

				uint32_t rtpSenderId = Utils::Crypto::GetRandomUInt(10000000, 99999999);
				RTC::RtpSender* rtpSender = new RTC::RtpSender(peer, this->notifier, rtpSenderId, rtpReceiver->kind);

				// Store into the maps.
				this->mapRtpReceiverRtpSenders[rtpReceiver].insert(rtpSender);
				this->mapRtpSenderRtpReceiver[rtpSender] = rtpReceiver;

				// Attach the RtpSender to peer.
				peer->AddRtpSender(rtpSender, receiver_peer->peerName, rtpReceiver->GetParameters());

				// Provide the RtpSender with the parameters of the RtpReceiver.
				rtpSender->Send(rtpReceiver->GetParameters());
			}
		}
	}

	void Room::onPeerRtpReceiverParameters(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		MS_ASSERT(rtpReceiver->GetParameters(), "rtpReceiver->GetParameters() returns no RtpParameters");

		// If this is a new RtpReceiver, iterate all the peers but this one and
		// create a RtpSender associated to this RtpReceiver for each Peer.
		if (this->mapRtpReceiverRtpSenders.find(rtpReceiver) == this->mapRtpReceiverRtpSenders.end())
		{
			// Ensure the entry will exist even with an empty array.
			this->mapRtpReceiverRtpSenders[rtpReceiver];

			for (auto& kv : this->peers)
			{
				RTC::Peer* sender_peer = kv.second;

				// Skip receiver peer.
				if (sender_peer == peer)
					continue;

				// Skip peer with capabilities unset yet.
				if (!sender_peer->HasCapabilities())
					continue;

				// Create a RtpSender for the other Peer.
				uint32_t rtpSenderId = Utils::Crypto::GetRandomUInt(10000000, 99999999);
				RTC::RtpSender* rtpSender = new RTC::RtpSender(sender_peer, this->notifier, rtpSenderId, rtpReceiver->kind);

				// Store into the maps.
				this->mapRtpReceiverRtpSenders[rtpReceiver].insert(rtpSender);
				this->mapRtpSenderRtpReceiver[rtpSender] = rtpReceiver;

				// Attach the RtpSender to sender_peer.
				sender_peer->AddRtpSender(rtpSender, peer->peerName, rtpReceiver->GetParameters());
			}
		}
		// If this is not a new RtpReceiver let's retrieve its updated parameters
		// and update with them all the associated RtpSenders.
		else
		{
			for (auto rtpSender : this->mapRtpReceiverRtpSenders[rtpReceiver])
			{
				// Provide the RtpSender with the parameters of the RtpReceiver.
				rtpSender->Send(rtpReceiver->GetParameters());
			}
		}
	}

	void Room::onPeerRtpReceiverClosed(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// If the RtpReceiver is in the map, iterate the map and close all the
		// RtpSenders associated to the closed RtpReceiver.
		if (this->mapRtpReceiverRtpSenders.find(rtpReceiver) != this->mapRtpReceiverRtpSenders.end())
		{
			// Make a copy of the set of RtpSenders given that Close() will be called
			// in all of them, producing onPeerRtpSenderClosed() that will remove it
			// from the map.
			auto rtpSenders = this->mapRtpReceiverRtpSenders[rtpReceiver];

			// Safely iterate the copy of the set.
			for (auto& rtpSender : rtpSenders)
			{
				rtpSender->Close();
			}

			// Finally delete the RtpReceiver entry in the map.
			this->mapRtpReceiverRtpSenders.erase(rtpReceiver);
		}
	}

	void Room::onPeerRtpSenderClosed(RTC::Peer* peer, RTC::RtpSender* rtpSender)
	{
		MS_TRACE();

		// Iterate all the receiver/senders map and remove the closed RtpSender from all the
		// RtpReceiver entries.
		for (auto& kv : this->mapRtpReceiverRtpSenders)
		{
			auto& rtpSenders = kv.second;

			rtpSenders.erase(rtpSender);
		}

		// Also remove the entry from the sender/receiver map.
		this->mapRtpSenderRtpReceiver.erase(rtpSender);
	}

	void Room::onPeerRtpPacket(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(this->mapRtpReceiverRtpSenders.find(rtpReceiver) != this->mapRtpReceiverRtpSenders.end(), "RtpReceiver not present in the map");

		auto& rtpSenders = this->mapRtpReceiverRtpSenders[rtpReceiver];

		// Send the RtpPacket to all the RtpSenders associated to the RtpReceiver
		// from which it was received.
		for (auto& rtpSender : rtpSenders)
		{
			rtpSender->SendRtpPacket(packet);
		}
	}

	void Room::onPeerRtcpReceiverReport(RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		MS_ASSERT(this->mapRtpSenderRtpReceiver.find(rtpSender) != this->mapRtpSenderRtpReceiver.end(), "RtpSender not present in the map");

		auto& rtpReceiver = this->mapRtpSenderRtpReceiver[rtpSender];

		rtpReceiver->ReceiveRtcpReceiverReport(report);
	}

	void Room::onPeerRtcpSenderReport(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RTCP::SenderReport* report)
	{
		MS_TRACE();

		MS_ASSERT(this->mapRtpReceiverRtpSenders.find(rtpReceiver) != this->mapRtpReceiverRtpSenders.end(), "RtpReceiver not present in the map");

		auto& rtpSenders = this->mapRtpReceiverRtpSenders[rtpReceiver];

		for (auto& rtpSender : rtpSenders)
		{
			rtpSender->ReceiveRtcpSenderReport(report);
		}
	}

	void Room::onPeerRtcpSdesChunk(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RTCP::SdesChunk* chunk)
	{
		MS_TRACE();

		MS_ASSERT(this->mapRtpReceiverRtpSenders.find(rtpReceiver) != this->mapRtpReceiverRtpSenders.end(), "RtpReceiver not present in the map");

		auto& rtpSenders = this->mapRtpReceiverRtpSenders[rtpReceiver];

		for (auto& rtpSender : rtpSenders)
		{
			rtpSender->ReceiveRtcpSdesChunk(chunk);
		}
	}

	void Room::onPeerRtcpFeedback(RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackPsPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(this->mapRtpSenderRtpReceiver.find(rtpSender) != this->mapRtpSenderRtpReceiver.end(), "RtpSender not present in the map");

		auto& rtpReceiver = this->mapRtpSenderRtpReceiver[rtpSender];

		rtpReceiver->ReceiveRtcpFeedback(packet);
	}

	void Room::onPeerRtcpFeedback(RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackRtpPacket* packet)
	{
		MS_TRACE();

		MS_ASSERT(this->mapRtpSenderRtpReceiver.find(rtpSender) != this->mapRtpSenderRtpReceiver.end(), "RtpSender not present in the map");

		auto& rtpReceiver = this->mapRtpSenderRtpReceiver[rtpSender];

		switch (packet->GetMessageType())
		{
			case RTC::RTCP::FeedbackRtp::NACK:
			{
				RTC::RTCP::FeedbackRtpNackPacket* nackPacket = static_cast<RTC::RTCP::FeedbackRtpNackPacket*>(packet);

				for (auto it = nackPacket->Begin(); it != nackPacket->End(); it++)
				{
					RTC::RTCP::NackItem* item = *it;

					rtpReceiver->RequestRtpRetransmission(item->GetPacketId(), item->GetLostPacketBitmask(), this->rtpRetransmissionContainer);

					for (auto it = this->rtpRetransmissionContainer.begin(); it != this->rtpRetransmissionContainer.end(); it++)
					{
						RTC::RtpPacket* packet = *it;

						if (packet == nullptr)
							break;

						rtpSender->RetransmitRtpPacket(packet);
					}
				}

				break;
			}

			default:
			{
				rtpReceiver->ReceiveRtcpFeedback(packet);
			}
		}
	}

	void Room::onPeerRtcpCompleted(RTC::Peer* peer)
	{
		MS_TRACE();

		// Tell all the peers but the one in the argument to generate and send their RTCP
		for (auto it = this->peers.begin(); it != this->peers.end(); ++it)
		{
			if (it->second != peer) {
				it->second->SendRtcp();
			}
		}
	}
}
