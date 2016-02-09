#define MS_CLASS "RTC::Room"

#include "RTC/Room.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include "Utils.h"
#include <string>

namespace RTC
{
	/* Instance methods. */

	Room::Room(Listener* listener, Channel::Notifier* notifier, uint32_t roomId) :
		roomId(roomId),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();
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
		// removes it from the map, so this is the safe way to iterate the mao
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

		static const Json::StaticString k_peers("peers");

		Json::Value json(Json::objectValue);
		Json::Value json_peers(Json::objectValue);

		for (auto& kv : this->peers)
		{
			RTC::Peer* peer = kv.second;

			json_peers[std::to_string(peer->peerId)] = peer->toJson();
		}
		json[k_peers] = json_peers;

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
					MS_ERROR("Peer already exists");

					request->Reject("Peer already exists");
					return;
				}

				if (!request->internal[k_peerName].isString())
				{
					MS_ERROR("Request has not string `internal.peerName`");

					request->Reject("Request has not string `internal.peerName`");
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

				// Get all the RtpReceivers of the others Peers in the Room and create
				// RtpSenders for this new Peer.
				for (auto& kv : this->peers)
				{
					RTC::Peer* other_peer = kv.second;

					for (auto rtpReceiver : other_peer->GetRtpReceivers())
					{
						uint32_t rtpSenderId = Utils::Crypto::GetRandomUInt(10000000, 99999999);
						RTC::RtpSender* rtpSender = new RTC::RtpSender(peer, this->notifier, rtpSenderId);

						// Clone receiver RtpParameters.
						RTC::RtpParameters* rtpSenderParameters = new RTC::RtpParameters(rtpReceiver->GetRtpParameters());

						rtpSender->Send(rtpSenderParameters);

						// Attach the RtpSender to peer.
						peer->AddRtpSender(rtpSender);
					}
				}

				// Store the new Peer.
				this->peers[peerId] = peer;

				MS_DEBUG("Peer created [peerId:%u, peerName:'%s']", peerId, peerName.c_str());
				request->Accept();

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
			case Channel::Request::MethodId::rtpSender_close:
			case Channel::Request::MethodId::rtpSender_dump:
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
					MS_ERROR("Peer does not exist");

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

	void Room::onPeerRtpReceiverReady(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		static const Json::StaticString k_sender("sender");
		static const Json::StaticString k_receiver("receiver");
		static const Json::StaticString k_peerId("peerId");
		static const Json::StaticString k_rtpSenderId("rtpSenderId");
		static const Json::StaticString k_rtpReceiverId("rtpReceiverId");
		static const Json::StaticString k_rtpParameters("rtpParameters");

		MS_ASSERT(rtpReceiver->GetRtpParameters(), "rtpReceiver->GetRtpParameters() returns no RtpParameters");

		for (auto& kv : this->peers)
		{
			RTC::Peer* other_peer = kv.second;

			// Skip receiver peer.
			if (other_peer == peer)
				continue;

			// Create a RtpSender for each remaining Peer in the Room.

			uint32_t rtpSenderId = Utils::Crypto::GetRandomUInt(10000000, 99999999);
			RTC::RtpSender* rtpSender = new RTC::RtpSender(other_peer, this->notifier, rtpSenderId);

			// Clone receiver RtpParameters.
			RTC::RtpParameters* rtpSenderParameters = new RTC::RtpParameters(rtpReceiver->GetRtpParameters());

			rtpSender->Send(rtpSenderParameters);

			// Attach the RtpSender to other_peer.
			other_peer->AddRtpSender(rtpSender);

			Json::Value event_data(Json::objectValue);
			Json::Value event_sender(Json::objectValue);
			Json::Value event_receiver(Json::objectValue);

			event_sender[k_peerId] = other_peer->peerId;
			event_sender[k_rtpSenderId] = rtpSender->rtpSenderId;
			event_sender[k_rtpParameters] = rtpSender->GetRtpParameters()->toJson();

			event_receiver[k_peerId] = peer->peerId;
			event_receiver[k_rtpReceiverId] = rtpReceiver->rtpReceiverId;

			event_data[k_sender] = event_sender;
			event_data[k_receiver] = event_receiver;

			this->notifier->Emit(this->roomId, "newrtpsender", event_data);
		}
	}

	void Room::onPeerRtpReceiverClosed(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

	}

	void Room::onPeerRtpSenderClosed(RTC::Peer* peer, RTC::RtpSender* rtpSender)
	{
		MS_TRACE();

	}
}
