#define MS_CLASS "RTC::Room"

#include "RTC/Room.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	Room::Room(unsigned int roomId, Json::Value& data) :
		roomId(roomId)
	{
		MS_TRACE();

		// TODO: do something with data and throw if incorrect.
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
			auto peer = it->second;

			it = this->peers.erase(it);
			peer->Close();
		}

		delete this;
	}

	Json::Value Room::Dump()
	{
		MS_TRACE();

		Json::Value json(Json::objectValue);
		Json::Value jsonPeers(Json::objectValue);

		for (auto& kv : this->peers)
		{
			auto peer = kv.second;

			jsonPeers[peer->peerId] = peer->Dump();
		}

		json["peers"] = jsonPeers;

		return json;
	}

	void Room::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::createPeer:
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
					peer = new RTC::Peer(this, peerId, request->data);
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

			case Channel::Request::MethodId::closePeer:
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

				if (!peer)
				{
					MS_ERROR("Peer does not exist");

					request->Reject(500, "Peer does not exist");
					return;
				}

				peer->Close();
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::createTransport:
			case Channel::Request::MethodId::createAssociatedTransport:
			case Channel::Request::MethodId::closeTransport:
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

		auto jsonPeerId = request->data["peerId"];

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
