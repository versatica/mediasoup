#ifndef MS_CHANNEL_MESSAGE_REGISTRATOR_HPP
#define MS_CHANNEL_MESSAGE_REGISTRATOR_HPP

#include "common.hpp"
#include "Channel/ChannelSocket.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include <absl/container/flat_hash_map.h>
#include <nlohmann/json.hpp>
#include <string>

class ChannelMessageRegistrator
{
public:
	explicit ChannelMessageRegistrator();
	~ChannelMessageRegistrator();

public:
	void FillJson(json& jsonObject);
	void RegisterHandler(
	  const std::string& id,
	  Channel::ChannelSocket::RequestHandler* channelRequestHandler,
	  PayloadChannel::PayloadChannelSocket::RequestHandler* payloadChannelRequestHandler,
	  PayloadChannel::PayloadChannelSocket::NotificationHandler* payloadChannelNotificationHandler);
	void UnregisterHandler(const std::string& id);
	Channel::ChannelSocket::RequestHandler* GetChannelRequestHandler(const std::string& id);
	PayloadChannel::PayloadChannelSocket::RequestHandler* GetPayloadChannelRequestHandler(
	  const std::string& id);
	PayloadChannel::PayloadChannelSocket::NotificationHandler* GetPayloadChannelNotificationHandler(
	  const std::string& id);

private:
	absl::flat_hash_map<std::string, Channel::ChannelSocket::RequestHandler*> mapChannelRequestHandlers;
	absl::flat_hash_map<std::string, PayloadChannel::PayloadChannelSocket::RequestHandler*>
	  mapPayloadChannelRequestHandlers;
	absl::flat_hash_map<std::string, PayloadChannel::PayloadChannelSocket::NotificationHandler*>
	  mapPayloadChannelNotificationHandlers;
};

#endif
