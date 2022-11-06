#define MS_CLASS "ChannelMessageHandlers"
// #define MS_LOG_DEV_LEVEL 3

#include "ChannelMessageHandlers.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

/* Class variables. */

thread_local absl::flat_hash_map<std::string, Channel::ChannelSocket::RequestHandler*>
  ChannelMessageHandlers::mapChannelRequestHandlers;
thread_local absl::flat_hash_map<std::string, PayloadChannel::PayloadChannelSocket::RequestHandler*>
  ChannelMessageHandlers::mapPayloadChannelRequestHandlers;
thread_local absl::flat_hash_map<std::string, PayloadChannel::PayloadChannelSocket::NotificationHandler*>
  ChannelMessageHandlers::mapPayloadChannelNotificationHandlers;

/* Class methods. */

void ChannelMessageHandlers::FillJson(json& jsonObject)
{
	MS_TRACE();

	// Add channelRequestHandlers.
	jsonObject["channelRequestHandlers"] = json::array();
	auto jsonChannelRequestHandlersIt    = jsonObject.find("channelRequestHandlers");

	for (const auto& kv : ChannelMessageHandlers::mapChannelRequestHandlers)
	{
		const auto& handlerId = kv.first;

		jsonChannelRequestHandlersIt->emplace_back(handlerId);
	}

	// Add payloadChannelRequestHandlers.
	jsonObject["payloadChannelRequestHandlers"] = json::array();
	auto jsonPayloadChannelRequestHandlersIt    = jsonObject.find("payloadChannelRequestHandlers");

	for (const auto& kv : ChannelMessageHandlers::mapPayloadChannelRequestHandlers)
	{
		const auto& handlerId = kv.first;

		jsonPayloadChannelRequestHandlersIt->emplace_back(handlerId);
	}

	// Add payloadChannelNotificationHandlers.
	jsonObject["payloadChannelNotificationHandlers"] = json::array();
	auto jsonPayloadChannelNotificationHandlersIt =
	  jsonObject.find("payloadChannelNotificationHandlers");

	for (const auto& kv : ChannelMessageHandlers::mapPayloadChannelNotificationHandlers)
	{
		const auto& handlerId = kv.first;

		jsonPayloadChannelNotificationHandlersIt->emplace_back(handlerId);
	}
}

void ChannelMessageHandlers::RegisterHandler(
  const std::string& id,
  Channel::ChannelSocket::RequestHandler* channelRequestHandler,
  PayloadChannel::PayloadChannelSocket::RequestHandler* payloadChannelRequestHandler,
  PayloadChannel::PayloadChannelSocket::NotificationHandler* payloadChannelNotificationHandler)
{
	MS_TRACE();

	if (channelRequestHandler != nullptr)
	{
		if (
		  ChannelMessageHandlers::mapChannelRequestHandlers.find(id) !=
		  ChannelMessageHandlers::mapChannelRequestHandlers.end())
		{
			MS_THROW_ERROR("Channel request handler with ID %s already exists", id.c_str());
		}

		ChannelMessageHandlers::mapChannelRequestHandlers[id] = channelRequestHandler;
	}

	if (payloadChannelRequestHandler != nullptr)
	{
		if (
		  ChannelMessageHandlers::mapPayloadChannelRequestHandlers.find(id) !=
		  ChannelMessageHandlers::mapPayloadChannelRequestHandlers.end())
		{
			if (channelRequestHandler != nullptr)
			{
				ChannelMessageHandlers::mapChannelRequestHandlers.erase(id);
			}

			MS_THROW_ERROR("PayloadChannel request handler with ID %s already exists", id.c_str());
		}

		ChannelMessageHandlers::mapPayloadChannelRequestHandlers[id] = payloadChannelRequestHandler;
	}

	if (payloadChannelNotificationHandler != nullptr)
	{
		if (
		  ChannelMessageHandlers::mapPayloadChannelNotificationHandlers.find(id) !=
		  ChannelMessageHandlers::mapPayloadChannelNotificationHandlers.end())
		{
			if (channelRequestHandler != nullptr)
			{
				ChannelMessageHandlers::mapChannelRequestHandlers.erase(id);
			}

			if (payloadChannelRequestHandler != nullptr)
			{
				ChannelMessageHandlers::mapPayloadChannelRequestHandlers.erase(id);
			}

			MS_THROW_ERROR("PayloadChannel notification handler with ID %s already exists", id.c_str());
		}

		ChannelMessageHandlers::mapPayloadChannelNotificationHandlers[id] =
		  payloadChannelNotificationHandler;
	}
}

void ChannelMessageHandlers::UnregisterHandler(const std::string& id)
{
	MS_TRACE();

	ChannelMessageHandlers::mapChannelRequestHandlers.erase(id);
	ChannelMessageHandlers::mapPayloadChannelRequestHandlers.erase(id);
	ChannelMessageHandlers::mapPayloadChannelNotificationHandlers.erase(id);
}

Channel::ChannelSocket::RequestHandler* ChannelMessageHandlers::GetChannelRequestHandler(
  const std::string& id)
{
	MS_TRACE();

	auto it = ChannelMessageHandlers::mapChannelRequestHandlers.find(id);

	if (it != ChannelMessageHandlers::mapChannelRequestHandlers.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

PayloadChannel::PayloadChannelSocket::RequestHandler* ChannelMessageHandlers::GetPayloadChannelRequestHandler(
  const std::string& id)
{
	MS_TRACE();

	auto it = ChannelMessageHandlers::mapPayloadChannelRequestHandlers.find(id);

	if (it != ChannelMessageHandlers::mapPayloadChannelRequestHandlers.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

PayloadChannel::PayloadChannelSocket::NotificationHandler* ChannelMessageHandlers::
  GetPayloadChannelNotificationHandler(const std::string& id)
{
	MS_TRACE();

	auto it = ChannelMessageHandlers::mapPayloadChannelNotificationHandlers.find(id);

	if (it != ChannelMessageHandlers::mapPayloadChannelNotificationHandlers.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}
