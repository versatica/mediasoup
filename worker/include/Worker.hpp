#ifndef MS_WORKER_HPP
#define MS_WORKER_HPP

#include "common.hpp"
#include "Channel/Request.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include "PayloadChannel/Notification.hpp"
#include "PayloadChannel/Request.hpp"
#include "PayloadChannel/UnixStreamSocket.hpp"
#include "RTC/Router.hpp"
#include "handles/SignalsHandler.hpp"
#include <json.hpp>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

class Worker : public Channel::UnixStreamSocket::Listener,
               public PayloadChannel::UnixStreamSocket::Listener,
               public SignalsHandler::Listener
{
public:
	explicit Worker(Channel::UnixStreamSocket* channel, PayloadChannel::UnixStreamSocket* payloadChannel);
	~Worker();

private:
	void Close();
	void FillJson(json& jsonObject) const;
	void FillJsonResourceUsage(json& jsonObject) const;
	void SetNewRouterIdFromInternal(json& internal, std::string& routerId) const;
	RTC::Router* GetRouterFromInternal(json& internal) const;

	/* Methods inherited from Channel::lUnixStreamSocket::Listener. */
public:
	void OnChannelRequest(Channel::UnixStreamSocket* channel, Channel::Request* request) override;
	void OnChannelClosed(Channel::UnixStreamSocket* channel) override;

	/* Methods inherited from PayloadChannel::lUnixStreamSocket::Listener. */
public:
	void OnPayloadChannelNotification(
	  PayloadChannel::UnixStreamSocket* payloadChannel,
	  PayloadChannel::Notification* notification) override;
	void OnPayloadChannelRequest(
	  PayloadChannel::UnixStreamSocket* payloadChannel, PayloadChannel::Request* request) override;
	void OnPayloadChannelClosed(PayloadChannel::UnixStreamSocket* payloadChannel) override;

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

private:
	// Passed by argument.
	Channel::UnixStreamSocket* channel{ nullptr };
	PayloadChannel::UnixStreamSocket* payloadChannel{ nullptr };
	// Allocated by this.
	SignalsHandler* signalsHandler{ nullptr };
	std::unordered_map<std::string, RTC::Router*> mapRouters;
	// Others.
	bool closed{ false };
};

#endif
