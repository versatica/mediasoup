#ifndef MS_CHANNEL_MESSAGE_HANDLERS_HPP
#define MS_CHANNEL_MESSAGE_HANDLERS_HPP

#include "common.hpp"
#include "Channel/ChannelSocket.hpp"
#include "PayloadChannel/PayloadChannelSocket.hpp"
#include <absl/container/flat_hash_map.h>
#include <nlohmann/json.hpp>
#include <string>

class ChannelMessageHandlers
{
public:
	static void FillJson(json& jsonObject);
	static void RegisterHandler(
	  const std::string& id,
	  Channel::ChannelSocket::RequestHandler* channelRequestHandler,
	  PayloadChannel::PayloadChannelSocket::RequestHandler* payloadChannelRequestHandler,
	  PayloadChannel::PayloadChannelSocket::NotificationHandler* payloadChannelNotificationHandler);
	static void UnregisterHandler(const std::string& id);
	static Channel::ChannelSocket::RequestHandler* GetChannelRequestHandler(const std::string& id);
	static PayloadChannel::PayloadChannelSocket::RequestHandler* GetPayloadChannelRequestHandler(
	  const std::string& id);
	static PayloadChannel::PayloadChannelSocket::NotificationHandler* GetPayloadChannelNotificationHandler(
	  const std::string& id);

private:
	thread_local static absl::flat_hash_map<std::string, Channel::ChannelSocket::RequestHandler*>
	  mapChannelRequestHandlers;
	thread_local static absl::flat_hash_map<std::string, PayloadChannel::PayloadChannelSocket::RequestHandler*>
	  mapPayloadChannelRequestHandlers;
	thread_local static absl::flat_hash_map<std::string, PayloadChannel::PayloadChannelSocket::NotificationHandler*>
	  mapPayloadChannelNotificationHandlers;
};

#endif
