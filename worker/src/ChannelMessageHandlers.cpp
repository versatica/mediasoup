#define MS_CLASS "ChannelMessageHandlers"
// #define MS_LOG_DEV_LEVEL 3

#include "ChannelMessageHandlers.hpp"
#include "FBS/worker_generated.h"
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

flatbuffers::Offset<FBS::Worker::ChannelMessageHandlers> ChannelMessageHandlers::FillBuffer(
  flatbuffers::FlatBufferBuilder& builder)
{
	// Add channelRequestHandlerIds.
	std::vector<flatbuffers::Offset<flatbuffers::String>> channelRequestHandlerIds;
	for (const auto& kv : ChannelMessageHandlers::mapChannelRequestHandlers)
	{
		const auto& handlerId = kv.first;

		channelRequestHandlerIds.push_back(builder.CreateString(handlerId));
	}

	// Add payloadChannelRequestHandlerIds.
	std::vector<flatbuffers::Offset<flatbuffers::String>> payloadChannelRequestHandlerIds;
	for (const auto& kv : ChannelMessageHandlers::mapPayloadChannelRequestHandlers)
	{
		const auto& handlerId = kv.first;

		channelRequestHandlerIds.push_back(builder.CreateString(handlerId));
	}

	// Add payloadChannelNotificationHandlerIds.
	std::vector<flatbuffers::Offset<flatbuffers::String>> payloadChannelNotificationHandlerIds;
	for (const auto& kv : ChannelMessageHandlers::mapPayloadChannelNotificationHandlers)
	{
		const auto& handlerId = kv.first;

		payloadChannelNotificationHandlerIds.push_back(builder.CreateString(handlerId));
	}

	return FBS::Worker::CreateChannelMessageHandlersDirect(
	  builder,
	  &channelRequestHandlerIds,
	  &payloadChannelRequestHandlerIds,
	  &payloadChannelNotificationHandlerIds);
}

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

	try
	{
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
				MS_THROW_ERROR("PayloadChannel notification handler with ID %s already exists", id.c_str());
			}

			ChannelMessageHandlers::mapPayloadChannelNotificationHandlers[id] =
			  payloadChannelNotificationHandler;
		}
	}
	catch (const MediaSoupError& error)
	{
		// In case of error unregister everything.
		ChannelMessageHandlers::UnregisterHandler(id);

		throw;
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
