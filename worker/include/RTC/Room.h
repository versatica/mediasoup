#ifndef MS_RTC_ROOM_H
#define MS_RTC_ROOM_H

#include "RTC/Peer.h"
#include "RTC/RTPPacket.h"
#include "RTC/RTCPPacket.h"
#include <vector>

namespace RTC
{
	class Room : public RTC::Peer::Listener
	{
	public:
		Room();
		virtual ~Room();

		void Close();

	/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		virtual void onRTPPacket(RTC::Peer* peer, RTC::RTPPacket* packet) override;
		virtual void onRTCPPacket(RTC::Peer* peer, RTC::RTCPPacket* packet) override;

	private:
		// Others.
		typedef std::vector<RTC::Peer*> Peers;
		Peers peers;
	};
}

#endif
