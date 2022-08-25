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
				MS_THROW_ERROR_STD("Channel request handler with ID %s already exists", id.c_str());
			}

			ChannelMessageHandlers::mapChannelRequestHandlers[id] = channelRequestHandler;
		}

		if (payloadChannelRequestHandler != nullptr)
		{
			if (
			  ChannelMessageHandlers::mapPayloadChannelRequestHandlers.find(id) !=
			  ChannelMessageHandlers::mapPayloadChannelRequestHandlers.end())
			{
				MS_THROW_ERROR_STD("PayloadChannel request handler with ID %s already exists", id.c_str());
			}

			ChannelMessageHandlers::mapPayloadChannelRequestHandlers[id] = payloadChannelRequestHandler;
		}

		if (payloadChannelNotificationHandler != nullptr)
		{
			if (
			  ChannelMessageHandlers::mapPayloadChannelNotificationHandlers.find(id) !=
			  ChannelMessageHandlers::mapPayloadChannelNotificationHandlers.end())
			{
				MS_THROW_ERROR_STD(
				  "PayloadChannel notification handler with ID %s already exists", id.c_str());
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
