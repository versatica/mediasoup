#ifndef MS_RTC_ROOM_H
#define MS_RTC_ROOM_H

#include "RTC/Peer.h"
#include "Channel/Request.h"
#include <string>
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class Room : public RTC::Peer::Listener
	{
	public:
		Room(unsigned int roomId, Json::Value& data);
		virtual ~Room();

		void Close();
		Json::Value Dump();
		void HandleRequest(Channel::Request* request);

	private:
		RTC::Peer* GetPeerFromRequest(Channel::Request* request, std::string* peerId = nullptr);

	/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		virtual void onPeerClosed(RTC::Peer* peer) override;

	public:
		// Passed by argument.
		unsigned int roomId;

	private:
		// Others.
		typedef std::unordered_map<std::string, RTC::Peer*> Peers;
		Peers peers;
	};
}

#endif
