#ifndef MS_WORKER_HPP
#define MS_WORKER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "PayloadChannel/Notification.hpp"
#include "PayloadChannel/PayloadChannelRequest.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include "RTC/Router.hpp"
#include "handles/SignalsHandler.hpp"
#include <absl/container/flat_hash_map.h>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class Worker : public Channel::ChannelSocket::Listener,
               public PayloadChannel::PayloadChannelSocket::Listener,
               public SignalsHandler::Listener
{
public:
	explicit Worker(Channel::ChannelSocket* channel, PayloadChannel::PayloadChannelSocket* payloadChannel);
	~Worker();

private:
	void Close();
	void FillJson(json& jsonObject) const;
	void FillJsonResourceUsage(json& jsonObject) const;
	void SetNewRouterIdFromInternal(json& internal, std::string& routerId) const;
	RTC::Router* GetRouterFromInternal(json& internal) const;

	/* Methods inherited from Channel::lUnixStreamSocket::Listener. */
public:
	void OnChannelRequest(Channel::ChannelSocket* channel, Channel::ChannelRequest* request) override;
	void OnChannelClosed(Channel::ChannelSocket* channel) override;

	/* Methods inherited from PayloadChannel::lUnixStreamSocket::Listener. */
public:
	void OnPayloadChannelNotification(
	  PayloadChannel::PayloadChannelSocket* payloadChannel,
	  PayloadChannel::Notification* notification) override;
	void OnPayloadChannelRequest(
	  PayloadChannel::PayloadChannelSocket* payloadChannel,
	  PayloadChannel::PayloadChannelRequest* request) override;
	void OnPayloadChannelClosed(PayloadChannel::PayloadChannelSocket* payloadChannel) override;

	/* Methods inherited from SignalsHandler::Listener. */
public:
	void OnSignal(SignalsHandler* signalsHandler, int signum) override;

private:
	// Passed by argument.
	Channel::ChannelSocket* channel{ nullptr };
	PayloadChannel::PayloadChannelSocket* payloadChannel{ nullptr };
	// Allocated by this.
	SignalsHandler* signalsHandler{ nullptr };
	absl::flat_hash_map<std::string, RTC::Router*> mapRouters;
	// Others.
	bool closed{ false };
};

#endif
