#ifndef MS_CHANNEL_UNIX_STREAM_SOCKET_H
#define MS_CHANNEL_UNIX_STREAM_SOCKET_H

#include "common.h"
#include "handles/UnixStreamSocket.h"

namespace Channel
{
	class UnixStreamSocket : public ::UnixStreamSocket
	{
	public:
		class Listener
		{
		public:
			// virtual void onControlMessage(Channel::UnixStreamSocket* unixSocket, const MS_BYTE* raw, size_t len) = 0;
			// virtual void onControlUnixStreamSocketClosed(Channel::UnixStreamSocket* unixSocket, bool is_closed_by_peer) = 0;
		};

	public:
		UnixStreamSocket(Listener* listener, int fd);
		virtual ~UnixStreamSocket();

	/* Pure virtual methods inherited from ::UnixStreamSocket. */
	public:
		virtual void userOnUnixStreamRead(const MS_BYTE* data, size_t len) override;
		virtual void userOnUnixStreamSocketClosed(bool is_closed_by_peer) override;

	private:
		// Passed by argument:
		Listener* listener = nullptr;
	};
}

#endif
