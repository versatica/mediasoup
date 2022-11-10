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

void ChannelMessageRegistrator::FillJson(json& jsonObject)
{
	MS_TRACE();

	// Add channelRequestHandlers.
	jsonObject["channelRequestHandlers"] = json::array();
	auto jsonChannelRequestHandlersIt    = jsonObject.find("channelRequestHandlers");

	for (const auto& kv : this->mapChannelRequestHandlers)
	{
		const auto& handlerId = kv.first;

		jsonChannelRequestHandlersIt->emplace_back(handlerId);
	}

	// Add payloadChannelRequestHandlers.
	jsonObject["payloadChannelRequestHandlers"] = json::array();
	auto jsonPayloadChannelRequestHandlersIt    = jsonObject.find("payloadChannelRequestHandlers");

	for (const auto& kv : this->mapPayloadChannelRequestHandlers)
	{
		const auto& handlerId = kv.first;

		jsonPayloadChannelRequestHandlersIt->emplace_back(handlerId);
	}

	// Add payloadChannelNotificationHandlers.
	jsonObject["payloadChannelNotificationHandlers"] = json::array();
	auto jsonPayloadChannelNotificationHandlersIt =
	  jsonObject.find("payloadChannelNotificationHandlers");

	for (const auto& kv : this->mapPayloadChannelNotificationHandlers)
	{
		const auto& handlerId = kv.first;

		jsonPayloadChannelNotificationHandlersIt->emplace_back(handlerId);
	}
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
