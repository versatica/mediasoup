#ifndef MS_RTC_PEER_H
#define MS_RTC_PEER_H

#include "RTC/ICETransport.h"
#include "RTC/RTPPacket.h"
#include "RTC/RTCPPacket.h"
#include "Channel/Request.h"
#include <string>
#include <json/json.h>

namespace RTC
{
	class Peer :
		public RTC::ICETransport::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onPeerClosed(RTC::Peer* peer) = 0;
		};

	public:
		Peer(Listener* listener, std::string& peerId, Json::Value& data);
		virtual ~Peer();

		void Close();
		Json::Value Dump();
		void HandleRequest(Channel::Request* request);

	/* Pure virtual methods inherited from RTC::ICETransport::Listener. */
	public:
		// TODO

	public:
		// Passed by argument.
		std::string peerId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Others.
		// TODO: it must be a unordered_map.
		RTC::ICETransport* iceTransport = nullptr;
	};
}

#endif
