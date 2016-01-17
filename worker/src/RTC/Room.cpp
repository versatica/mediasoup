#define MS_CLASS "RTC::Room"

#include "RTC/Room.h"
#include "MediaSoupError.h"
#include "Logger.h"
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

				this->peers[peerId] = peer;

				MS_DEBUG("Peer created [peerId:%u, peerName:'%s']", peerId, peerName.c_str());
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::peer_close:
			case Channel::Request::MethodId::peer_dump:
			case Channel::Request::MethodId::peer_createTransport:
			case Channel::Request::MethodId::peer_createAssociatedTransport:
			case Channel::Request::MethodId::peer_createRtpReceiver:
			case Channel::Request::MethodId::transport_close:
			case Channel::Request::MethodId::transport_dump:
			case Channel::Request::MethodId::transport_setRemoteDtlsParameters:
			case Channel::Request::MethodId::rtpReceiver_close:
			case Channel::Request::MethodId::rtpReceiver_dump:
			case Channel::Request::MethodId::rtpReceiver_receive:
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
}
