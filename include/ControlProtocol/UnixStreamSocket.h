#ifndef MS_CONTROL_PROTOCOL_UNIX_STREAM_SOCKET_H
#define MS_CONTROL_PROTOCOL_UNIX_STREAM_SOCKET_H


#include "common.h"
#include "handles/UnixStreamSocket.h"
#include "ControlProtocol/Parser.h"
#include "ControlProtocol/Message.h"


namespace ControlProtocol {


class UnixStreamSocket : public ::UnixStreamSocket {
public:
	class Listener {
	public:
		virtual void onControlProtocolMessage(ControlProtocol::UnixStreamSocket* unixSocket, ControlProtocol::Message* msg, const MS_BYTE* raw, size_t len) = 0;
		virtual void onControlProtocolUnixStreamSocketClosed(ControlProtocol::UnixStreamSocket* unixSocket, bool is_closed_by_peer) = 0;
	};

public:
	UnixStreamSocket(Listener* listener, int fd);
	virtual ~UnixStreamSocket();

/* Pure virtual methods inherited from ::UnixStreamSocket. */
public:
	virtual void userOnUnixStreamRead(const MS_BYTE* data, size_t len) override;
	virtual void userOnUnixStreamSocketClosed(bool is_closed_by_peer) override;

private:
	// Allocated by this:
	ControlProtocol::Parser* parser = nullptr;
	// Passed by argument:
	Listener* listener = nullptr;
	// Others:
	size_t msgStart = 0;  // Where the latest message starts.
};


}  // namespace ControlProtocol


#endif
