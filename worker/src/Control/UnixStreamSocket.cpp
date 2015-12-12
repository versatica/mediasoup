#define MS_CLASS "Control::UnixStreamSocket"

#include "Control/UnixStreamSocket.h"
#include "Logger.h"

#define MESSAGE_MAX_SIZE 65536  // TODO: set proper buffer size

namespace Control
{
	/* Instance methods. */

	UnixStreamSocket::UnixStreamSocket(Listener* listener, int fd) :
		::UnixStreamSocket::UnixStreamSocket(fd, MESSAGE_MAX_SIZE),
		listener(listener)
	{
		MS_TRACE();
	}

	UnixStreamSocket::~UnixStreamSocket()
	{
		MS_TRACE();
	}

	void UnixStreamSocket::userOnUnixStreamRead(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		std::string kk((const char*)data, len);

		MS_WARN("---------- readed kk: %s", kk.c_str());

		this->bufferDataLen = 0;
	}

	void UnixStreamSocket::userOnUnixStreamSocketClosed(bool is_closed_by_peer)
	{
		MS_TRACE();

		// Notify the listener.
		// this->listener->onControlUnixStreamSocketClosed(this, is_closed_by_peer);
	}
}  // namespace ControlProtocol
