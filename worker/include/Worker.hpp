#ifndef MS_WORKER_HPP
#define MS_WORKER_HPP

#include "common.hpp"
#include "Channel/Request.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include "RTC/Router.hpp"
#include "handles/SignalsHandler.hpp"
#include <json.hpp>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

class Worker : public Channel::UnixStreamSocket::Listener, public SignalsHandler::Listener
{
public:
	explicit Worker(Channel::UnixStreamSocket* channel);
	~Worker();

private:
	void Close();
	void FillJson(json& jsonObject) const;
	void SetNewRouterIdFromRequest(Channel::Request* request, std::string& routerId) const;
	RTC::Router* GetRouterFromRequest(Channel::Request* request) const;

	/* Methods inherited from Channel::lUnixStreamSocket::Listener. */
public:
	void OnChannelRequest(Channel::UnixStreamSocket* channel, Channel::Request* request) override;
	void OnChannelClosed(Channel::UnixStreamSocket* channel) override;

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

private:
	// Passed by argument.
	Channel::UnixStreamSocket* channel{ nullptr };
	// Allocated by this.
	SignalsHandler* signalsHandler{ nullptr };
	std::unordered_map<std::string, RTC::Router*> mapRouters;
	// Others.
	bool closed{ false };
};

#endif
