#ifndef MS_WORKER_H
#define	MS_WORKER_H

#include "common.h"
#include "handles/SignalsHandler.h"
#include "Channel/UnixStreamSocket.h"
#include "Channel/Request.h"
#include "RTC/Room.h"
#include <unordered_map>

class Loop :
	public SignalsHandler::Listener,
	public Channel::UnixStreamSocket::Listener
{
public:
	Loop();
	~Loop();

private:
	void HandleCreateRoomRequest(Channel::Request* request);
	void Close();

/* Methods inherited from SignalsHandler::Listener. */
public:
	virtual void onSignal(SignalsHandler* signalsHandler, int signum) override;
	virtual void onSignalsHandlerClosed(SignalsHandler* signalsHandler) override;

/* Methods inherited from Channel::lUnixStreamSocket::Listener. */
public:
	virtual void onChannelRequest(Channel::UnixStreamSocket* channel, Channel::Request* request) override;
	virtual void onChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* channel) override;

private:
	// Allocated by this.
	SignalsHandler* signalsHandler = nullptr;
	Channel::UnixStreamSocket* channel = nullptr;
	// Others.
	bool closed = false;
	typedef std::unordered_map<unsigned int, RTC::Room*> Rooms;
	Rooms rooms;
};

#endif
