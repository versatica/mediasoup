#ifndef MS_RTC_PEER_H
#define MS_RTC_PEER_H

#include "RTC/Transport.h"
#include "RTC/RTPPacket.h"
#include "RTC/RTCPPacket.h"
#include <string>
#include <json/json.h>

namespace RTC
{
	class Peer : public RTC::Transport::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onRTPPacket(RTC::Peer* peer, RTC::RTPPacket* packet) = 0;
			virtual void onRTCPPacket(RTC::Peer* peer, RTC::RTCPPacket* packet) = 0;
		};

	public:
		Peer(Listener* listener, std::string& peerId, Json::Value& data);
		virtual ~Peer();

		void SendRTPPacket(RTC::RTPPacket* packet);
		void SendRTCPPacket(RTC::RTCPPacket* packet);
		void Close();

	/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		virtual void onRTPPacket(RTC::Transport* transport, RTC::RTPPacket* packet) override;
		virtual void onRTCPPacket(RTC::Transport* transport, RTC::RTCPPacket* packet) override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		std::string peerId;
		// Others.
		RTC::Transport* transport = nullptr;
	};
}

#endif
