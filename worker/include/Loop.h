#ifndef MS_WORKER_H
#define	MS_WORKER_H

#include "common.h"
#include "handles/SignalsHandler.h"
#include "Channel/UnixStreamSocket.h"

class Loop :
	public SignalsHandler::Listener,
	public Channel::UnixStreamSocket::Listener
{
public:
	Loop();
	~Loop();

	// TODO: IMHO this can be private.
	void Close();

/* Methods inherited from SignalsHandler::Listener. */
public:
	virtual void onSignal(SignalsHandler* signalsHandler, int signum) override;
	virtual void onSignalsHandlerClosed(SignalsHandler* signalsHandler) override;

/* Methods inherited from Channel::lUnixStreamSocket::Listener. */
public:
	virtual void onChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* unixSocket) override;

private:
	// Allocated by this.
	SignalsHandler* signalsHandler = nullptr;
	Channel::UnixStreamSocket* channel = nullptr;
	// Others.
	bool closed = false;
};

#endif
