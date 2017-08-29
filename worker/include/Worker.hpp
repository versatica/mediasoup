#ifndef MS_WORKER_HPP
#define MS_WORKER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include "RTC/Router.hpp"
#include "handles/SignalsHandler.hpp"
#include <unordered_map>

class Worker : public SignalsHandler::Listener,
               public Channel::UnixStreamSocket::Listener,
               public RTC::Router::Listener
{
public:
	explicit Worker(Channel::UnixStreamSocket* channel);
	~Worker() override;

private:
	void Close();
	RTC::Router* GetRouterFromRequest(Channel::Request* request, uint32_t* routerId = nullptr);

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

	/* Methods inherited from Channel::lUnixStreamSocket::Listener. */
public:
	void OnChannelRequest(Channel::UnixStreamSocket* channel, Channel::Request* request) override;
	void OnChannelUnixStreamSocketRemotelyClosed(Channel::UnixStreamSocket* channel) override;

	/* Methods inherited from RTC::Router::Listener. */
public:
	void OnRouterClosed(RTC::Router* router) override;

private:
	// Passed by argument.
	Channel::UnixStreamSocket* channel{ nullptr };
	// Allocated by this.
	Channel::Notifier* notifier{ nullptr };
	SignalsHandler* signalsHandler{ nullptr };
	// Others.
	bool closed{ false };
	std::unordered_map<uint32_t, RTC::Router*> routers;
};

#endif
