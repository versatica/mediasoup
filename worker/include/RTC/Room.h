#ifndef MS_RTC_ROOM_H
#define MS_RTC_ROOM_H

#include "RTC/Peer.h"
#include "RTC/RTPPacket.h"
#include "RTC/RTCPPacket.h"
#include "Channel/Request.h"
#include <string>
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class Room : public RTC::Peer::Listener
	{
	public:
		static RTC::Room* Factory(unsigned int roomId, Json::Value& data);

	public:
		Room(unsigned int roomId);
		virtual ~Room();

		void HandleCreatePeerRequest(Channel::Request* request);
		void HandleClosePeerRequest(Channel::Request* request);
		void Close();

	private:
		RTC::Peer* GetPeerFromRequest(Channel::Request* request, std::string* peerId);

	/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		virtual void onRTPPacket(RTC::Peer* peer, RTC::RTPPacket* packet) override;
		virtual void onRTCPPacket(RTC::Peer* peer, RTC::RTCPPacket* packet) override;

	private:
		// Passed by argument.
		unsigned int roomId;
		// Others.
		typedef std::unordered_map<std::string, RTC::Peer*> Peers;
		Peers peers;
	};
}

#endif
