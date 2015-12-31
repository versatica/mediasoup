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
	public Channel::UnixStreamSocket::Listener,
	public RTC::Room::Listener
{
public:
	Loop(Channel::UnixStreamSocket* channel);
	~Loop();

private:
	void Close();
	RTC::Room* GetRoomFromRequest(Channel::Request* request, unsigned int* roomId = nullptr);

/* Methods inherited from SignalsHandler::Listener. */
public:
	virtual void onSignal(SignalsHandler* signalsHandler, int signum) override;

/* Methods inherited from Channel::lUnixStreamSocket::Listener. */
public:
	virtual void onChannelRequest(Channel::UnixStreamSocket* channel, Channel::Request* request) override;
	virtual void onChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* channel) override;

/* Methods inherited from RTC::Room::Listener. */
public:
	virtual void onRoomClosed(RTC::Room* room) override;

private:
	// Passed by argument.
	Channel::UnixStreamSocket* channel = nullptr;
	// Allocated by this.
	SignalsHandler* signalsHandler = nullptr;
	// Others.
	bool closed = false;
	typedef std::unordered_map<unsigned int, RTC::Room*> Rooms;
	Rooms rooms;
};

#endif
