#ifndef MS_RTC_ROOM_H
#define MS_RTC_ROOM_H

#include "RTC/Peer.h"
#include "Channel/Request.h"
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class Room :
		public RTC::Peer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onRoomClosed(RTC::Room* room) = 0;
		};

	public:
		Room(Listener* listener, unsigned int roomId);
		virtual ~Room();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);

	private:
		RTC::Peer* GetPeerFromRequest(Channel::Request* request, unsigned int* peerId = nullptr);

	/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		virtual void onPeerClosed(RTC::Peer* peer) override;

	public:
		// Passed by argument.
		unsigned int roomId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Others.
		std::unordered_map<unsigned int, RTC::Peer*> peers;
	};
}

#endif
