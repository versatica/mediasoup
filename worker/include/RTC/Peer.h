#ifndef MS_RTC_PEER_H
#define MS_RTC_PEER_H

#include "RTC/Transport.h"
#include "Channel/Request.h"
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
		Peer(Listener* listener, std::string& peerId, Json::Value& data);
		virtual ~Peer();

		void Close();
		Json::Value Dump();
		void HandleRequest(Channel::Request* request);

	private:
		RTC::Transport* GetTransportFromRequest(Channel::Request* request, unsigned int* iceTransportId = nullptr);

	/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		virtual void onTransportClosed(RTC::Transport* transport) override;

	public:
		// Passed by argument.
		std::string peerId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Others.
		typedef std::unordered_map<unsigned int, RTC::Transport*> Transports;
		Transports transports;
	};
}

#endif
