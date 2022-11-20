#define MS_CLASS "ChannelMessageRegistrator"
// #define MS_LOG_DEV_LEVEL 3

#include "ChannelMessageRegistrator.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

ChannelMessageRegistrator::ChannelMessageRegistrator()
{
	MS_TRACE();
}

ChannelMessageRegistrator::~ChannelMessageRegistrator()
{
	MS_TRACE();

	this->mapChannelRequestHandlers.clear();
	this->mapPayloadChannelRequestHandlers.clear();
	this->mapPayloadChannelNotificationHandlers.clear();
}

flatbuffers::Offset<FBS::Worker::ChannelMessageHandlers> ChannelMessageRegistrator::FillBuffer(
  flatbuffers::FlatBufferBuilder& builder)
{
	// Add channelRequestHandlerIds.
	std::vector<flatbuffers::Offset<flatbuffers::String>> channelRequestHandlerIds;
	for (const auto& kv : this->mapChannelRequestHandlers)
	{
		const auto& handlerId = kv.first;

		channelRequestHandlerIds.push_back(builder.CreateString(handlerId));
	}

	// Add payloadChannelRequestHandlerIds.
	std::vector<flatbuffers::Offset<flatbuffers::String>> payloadChannelRequestHandlerIds;
	for (const auto& kv : this->mapPayloadChannelRequestHandlers)
	{
		const auto& handlerId = kv.first;

		channelRequestHandlerIds.push_back(builder.CreateString(handlerId));
	}

	// Add payloadChannelNotificationHandlerIds.
	std::vector<flatbuffers::Offset<flatbuffers::String>> payloadChannelNotificationHandlerIds;
	for (const auto& kv : this->mapPayloadChannelNotificationHandlers)
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

void ChannelMessageRegistrator::RegisterHandler(
  const std::string& id,
  Channel::ChannelSocket::RequestHandler* channelRequestHandler,
  PayloadChannel::PayloadChannelSocket::RequestHandler* payloadChannelRequestHandler,
  PayloadChannel::PayloadChannelSocket::NotificationHandler* payloadChannelNotificationHandler)
{
	MS_TRACE();

	if (channelRequestHandler != nullptr)
	{
		if (this->mapChannelRequestHandlers.find(id) != this->mapChannelRequestHandlers.end())
		{
			MS_THROW_ERROR("Channel request handler with ID %s already exists", id.c_str());
		}

		this->mapChannelRequestHandlers[id] = channelRequestHandler;
	}

	if (payloadChannelRequestHandler != nullptr)
	{
		if (this->mapPayloadChannelRequestHandlers.find(id) != this->mapPayloadChannelRequestHandlers.end())
		{
			if (channelRequestHandler != nullptr)
			{
				this->mapChannelRequestHandlers.erase(id);
			}

			MS_THROW_ERROR("PayloadChannel request handler with ID %s already exists", id.c_str());
		}

		this->mapPayloadChannelRequestHandlers[id] = payloadChannelRequestHandler;
	}

	if (payloadChannelNotificationHandler != nullptr)
	{
		if (
		  this->mapPayloadChannelNotificationHandlers.find(id) !=
		  this->mapPayloadChannelNotificationHandlers.end())
		{
			if (channelRequestHandler != nullptr)
			{
				this->mapChannelRequestHandlers.erase(id);
			}

			if (payloadChannelRequestHandler != nullptr)
			{
				this->mapPayloadChannelRequestHandlers.erase(id);
			}

			MS_THROW_ERROR("PayloadChannel notification handler with ID %s already exists", id.c_str());
		}

		this->mapPayloadChannelNotificationHandlers[id] = payloadChannelNotificationHandler;
	}
}

void ChannelMessageRegistrator::UnregisterHandler(const std::string& id)
{
	MS_TRACE();

	this->mapChannelRequestHandlers.erase(id);
	this->mapPayloadChannelRequestHandlers.erase(id);
	this->mapPayloadChannelNotificationHandlers.erase(id);
}

Channel::ChannelSocket::RequestHandler* ChannelMessageRegistrator::GetChannelRequestHandler(
  const std::string& id)
{
	MS_TRACE();

	auto it = this->mapChannelRequestHandlers.find(id);

	if (it != this->mapChannelRequestHandlers.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

PayloadChannel::PayloadChannelSocket::RequestHandler* ChannelMessageRegistrator::GetPayloadChannelRequestHandler(
  const std::string& id)
{
	MS_TRACE();

	auto it = this->mapPayloadChannelRequestHandlers.find(id);

	if (it != this->mapPayloadChannelRequestHandlers.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

PayloadChannel::PayloadChannelSocket::NotificationHandler* ChannelMessageRegistrator::
  GetPayloadChannelNotificationHandler(const std::string& id)
{
	MS_TRACE();

	auto it = this->mapPayloadChannelNotificationHandlers.find(id);

	if (it != this->mapPayloadChannelNotificationHandlers.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}
