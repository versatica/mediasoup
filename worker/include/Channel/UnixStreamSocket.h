#ifndef MS_CHANNEL_UNIX_STREAM_SOCKET_H
#define MS_CHANNEL_UNIX_STREAM_SOCKET_H

#include "common.h"
#include "handles/UnixStreamSocket.h"
#include "Channel/Request.h"
#include <json/json.h>

namespace Channel
{
	// Avoid cyclic #include problem.
	class Request;

	class UnixStreamSocket : public ::UnixStreamSocket
	{
	public:
		class Listener
		{
		public:
			virtual void onChannelRequest(Channel::UnixStreamSocket* channel, Channel::Request* request) = 0;
			virtual void onChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* channel) = 0;
		};

	private:
		static MS_BYTE writeBuffer[];

	public:
		UnixStreamSocket(Listener* listener, int fd);
		virtual ~UnixStreamSocket();

		void Send(Json::Value &json);

	/* Pure virtual methods inherited from ::UnixStreamSocket. */
	public:
		virtual void userOnUnixStreamRead() override;
		virtual void userOnUnixStreamSocketClosed(bool is_closed_by_peer) override;

	private:
		// Passed by argument:
		Listener* listener = nullptr;
		// Others:
		size_t msgStart = 0;  // Where the latest message starts.
	};
}

#endif
