#ifndef MS_RTC_PEER_H
#define MS_RTC_PEER_H

#include "common.h"
#include "RTC/Transport.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <string>
#include <unordered_map>
#include <json/json.h>

namespace RTC
{
	class Peer :
		public RTC::Transport::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onPeerClosed(RTC::Peer* peer) = 0;
		};

	public:
		Peer(Listener* listener, Channel::Notifier* notifier, uint32_t peerId, std::string& peerName);
		virtual ~Peer();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);

	private:
		RTC::Transport* GetTransportFromRequest(Channel::Request* request, uint32_t* iceTransportId = nullptr);

	/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		virtual void onTransportClosed(RTC::Transport* transport) override;

	public:
		// Passed by argument.
		uint32_t peerId;
		std::string peerName;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		// Others.
		std::unordered_map<uint32_t, RTC::Transport*> transports;
	};
}

#endif
