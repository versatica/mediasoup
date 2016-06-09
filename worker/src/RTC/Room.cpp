#define MS_CLASS "RTC::Room"

#include "RTC/Room.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include "Utils.h"
#include <string>

namespace RTC
{
	/* Class variables. */

	RTC::RtpCapabilities Room::supportedRtpCapabilities;

	/* Class methods. */

	void Room::ClassInit()
	{
		MS_TRACE();

		// Parse all RTP capabilities.
		{
			Json::CharReaderBuilder builder;
			Json::Value settings = Json::nullValue;
			Json::Value invalid_settings;

			builder.strictMode(&settings);

			Json::CharReader* jsonReader = builder.newCharReader();

			// NOTE: This line is auto-generated frmo data/supportedRtpCapabilities.js.
			const std::string capabilities = R"({"codecs":[{"kind":"audio","name":"audio/opus","preferredPayloadType":100,"clockRate":90000,"rtcpFeedback":[{"type":"ccm","parameter":"fir"},{"type":"nack","parameter":""},{"type":"nack","parameter":"pli"},{"type":"google-remb","parameter":""}]}]})";

			Json::Value json;
			std::string json_parse_error;

			if (!jsonReader->parse(capabilities.c_str(), capabilities.c_str() + capabilities.length(), &json, &json_parse_error))
			{
				delete jsonReader;

				MS_THROW_ERROR_STD("JSON parsing error in default RTP capabilities: %s", json_parse_error.c_str());
			}

			delete jsonReader;

			try
			{
				Room::supportedRtpCapabilities = RTC::RtpCapabilities(json);
			}
			catch (const MediaSoupError &error)
			{
				MS_THROW_ERROR_STD("wrong default RTP capabilities: %s", error.what());
			}
		}
	}

	/* Instance methods. */

	Room::Room(Listener* listener, Channel::Notifier* notifier, uint32_t roomId, Json::Value& data) :
		roomId(roomId),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();

		static const Json::StaticString k_mediaCodecs("mediaCodecs");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_name("name");

		// Initialize room RTP capabilities with all the supported ones.
		this->rtpCapabilities = Room::supportedRtpCapabilities;

		// `mediaCodecs` is mandatory.
		if (!data[k_mediaCodecs].isArray())
			MS_THROW_ERROR("missing mediaCodecs");

		auto& json_mediaCodecs = data[k_mediaCodecs];

		if (json_mediaCodecs.size() == 0)
			MS_THROW_ERROR("empty mediaCodecs");

		for (Json::UInt i = 0; i < json_mediaCodecs.size(); i++)
		{
			auto& json_mediaCodec = json_mediaCodecs[i];

			if (!json_mediaCodec[k_kind].isString())
				MS_THROW_ERROR("missing mediaCodec.kind");

			if (!json_mediaCodec[k_name].isString())
				MS_THROW_ERROR("missing mediaCodec.name");

			std::string str_kind = json_mediaCodec[k_kind].asString();
			std::string str_name = json_mediaCodec[k_name].asString();
			RTC::Media::Kind kind = RTC::Media::GetKind(str_kind);
			RTC::RtpCodecMime mime;

			mime.SetName(str_name);

			// TODO: more
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
		static const Json::StaticString k_peers("peers");
		static const Json::StaticString k_mapRtpReceiverRtpSenders("mapRtpReceiverRtpSenders");

		Json::Value json(Json::objectValue);
		Json::Value json_peers(Json::arrayValue);
		Json::Value json_mapRtpReceiverRtpSenders(Json::objectValue);

		// Add `roomId`.
		json[k_roomId] = (Json::UInt)this->roomId;

		// Add `peers`.
		for (auto& kv : this->peers)
		{
			RTC::Peer* peer = kv.second;

			json_peers.append(peer->toJson());
		}
		json[k_peers] = json_peers;

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

		// Add `mapRtpReceiverRtpSenders`.
		json[k_mapRtpReceiverRtpSenders] = json_mapRtpReceiverRtpSenders;

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
					peer = new RTC::Peer(this, this->notifier, peerId, peerName, request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				MS_DEBUG("Peer created [peerId:%u, peerName:'%s']", peerId, peerName.c_str());
				request->Accept();

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

						// Store into the map.
						this->mapRtpReceiverRtpSenders[rtpReceiver].insert(rtpSender);

						// Take the sender parameters of the receiver.
						rtpSender->Send(rtpReceiver->GetSenderParameters());

						// Attach the RtpSender to peer.
						peer->AddRtpSender(rtpSender, receiver_peer->peerName);
					}
				}

				// Store the new Peer.
				// NOTE: Do it after iterating existing Peers.
				this->peers[peerId] = peer;

				break;
			}

			case Channel::Request::MethodId::room_getCapabilities:
			{
				// TODO

				Json::Value json(Json::objectValue);

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::peer_close:
			case Channel::Request::MethodId::peer_dump:
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

	void Room::onPeerClosed(RTC::Peer* peer)
	{
		MS_TRACE();

		this->peers.erase(peer->peerId);
	}

	void Room::onPeerRtpReceiverParameters(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// auto rtpParameters = rtpReceiver->GetParameters();

		// TODO: Check codecs availability and, optionally, uniqueness of PTs.
		// If it fails throw.
	}

	void Room::onPeerRtpReceiverParametersDone(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver)
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

				// Create a RtpSender for the other Peer.
				uint32_t rtpSenderId = Utils::Crypto::GetRandomUInt(10000000, 99999999);
				RTC::RtpSender* rtpSender = new RTC::RtpSender(sender_peer, this->notifier, rtpSenderId, rtpReceiver->kind);

				// Store into the map.
				this->mapRtpReceiverRtpSenders[rtpReceiver].insert(rtpSender);

				// Take the sender parameters of the receiver.
				rtpSender->Send(rtpReceiver->GetSenderParameters());

				// Attach the RtpSender to sender_peer.
				sender_peer->AddRtpSender(rtpSender, peer->peerName);
			}
		}
		// If this is not a new RtpReceiver let's retrieve its updated parameters
		// and update with them all the associated RtpSenders.
		else
		{
			for (auto rtpSender : this->mapRtpReceiverRtpSenders[rtpReceiver])
			{
				// Take the sender parameters of the receiver.
				rtpSender->Send(rtpReceiver->GetSenderParameters());
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

		// Iterate all the map and remove the closed RtpSender from all the
		// RtpReceiver entries.
		for (auto& kv : this->mapRtpReceiverRtpSenders)
		{
			auto& rtpSenders = kv.second;

			rtpSenders.erase(rtpSender);
		}
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
}
