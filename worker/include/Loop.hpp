#ifndef MS_LOOP_HPP
#define MS_LOOP_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include "RTC/Room.hpp"
#include "handles/SignalsHandler.hpp"
#include <unordered_map>

class Loop : public SignalsHandler::Listener,
             public Channel::UnixStreamSocket::Listener,
             public RTC::Room::Listener
{
public:
	explicit Loop(Channel::UnixStreamSocket* channel);
	~Loop() override;

private:
	void Close();
	RTC::Room* GetRoomFromRequest(Channel::Request* request, uint32_t* roomId = nullptr);

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

	/* Methods inherited from Channel::lUnixStreamSocket::Listener. */
public:
	void OnChannelRequest(Channel::UnixStreamSocket* channel, Channel::Request* request) override;
	void OnChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* channel) override;

	/* Methods inherited from RTC::Room::Listener. */
public:
	void OnRoomClosed(RTC::Room* room) override;

private:
	// Passed by argument.
	Channel::UnixStreamSocket* channel{ nullptr };
	// Allocated by this.
	Channel::Notifier* notifier{ nullptr };
	SignalsHandler* signalsHandler{ nullptr };
	// Others.
	bool closed{ false };
	std::unordered_map<uint32_t, RTC::Room*> rooms;
};

#endif
