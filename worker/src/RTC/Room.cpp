#define MS_CLASS "RTC::Room"

#include "RTC/Room.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	Room::Room(Listener* listener, unsigned int roomId) :
		roomId(roomId),
		listener(listener)
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

			json_peers[peer->peerId] = peer->toJson();
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
				unsigned int roomId = this->roomId;

				Close();

				MS_DEBUG("Room closed [roomId:%u]", roomId);
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
				RTC::Peer* peer;
				std::string peerId;

				try
				{
					peer = GetPeerFromRequest(request, &peerId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				if (peer)
				{
					MS_ERROR("Peer already exists");

					request->Reject(500, "Peer already exists");
					return;
				}

				try
				{
					peer = new RTC::Peer(this, peerId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				this->peers[peerId] = peer;

				MS_DEBUG("Peer created [peerId:%s]", peerId.c_str());
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::peer_close:
			case Channel::Request::MethodId::peer_dump:
			case Channel::Request::MethodId::peer_createTransport:
			case Channel::Request::MethodId::peer_createAssociatedTransport:
			case Channel::Request::MethodId::transport_close:
			case Channel::Request::MethodId::transport_dump:
			case Channel::Request::MethodId::transport_start:
			{
				RTC::Peer* peer;

				try
				{
					peer = GetPeerFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				if (!peer)
				{
					MS_ERROR("Peer does not exist");

					request->Reject(500, "Peer does not exist");
					return;
				}

				peer->HandleRequest(request);

				break;
			}

			default:
			{
				MS_ABORT("unknown method");
			}
		}
	}

	RTC::Peer* Room::GetPeerFromRequest(Channel::Request* request, std::string* peerId)
	{
		MS_TRACE();

		static const Json::StaticString k_peerId("peerId");

		auto jsonPeerId = request->data[k_peerId];

		if (!jsonPeerId.isString())
			MS_THROW_ERROR("Request has not string .peerId field");

		// If given, fill peerId.
		if (peerId)
			*peerId = jsonPeerId.asString();

		auto it = this->peers.find(jsonPeerId.asString());

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
