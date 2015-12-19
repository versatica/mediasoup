#ifndef MS_CHANNEL_UNIX_STREAM_SOCKET_H
#define MS_CHANNEL_UNIX_STREAM_SOCKET_H

#include "common.h"
#include "handles/UnixStreamSocket.h"
#include <json/json.h>

namespace Channel
{
	class UnixStreamSocket : public ::UnixStreamSocket
	{
	public:
		class Listener
		{
		public:
			// virtual void onControlMessage(Channel::UnixStreamSocket* unixSocket, const MS_BYTE* raw, size_t len) = 0;
			virtual void onChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* unixSocket) = 0;
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

	// Channel singleton.
	extern UnixStreamSocket* channel;
}

#endif
