#define MS_CLASS "Channel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelNotifier.hpp"
#include "Logger.hpp"

namespace Channel
{
	ChannelNotifier::ChannelNotifier(Channel::ChannelSocket* channel) : channel(channel)
	{
		MS_TRACE();
	}

	void ChannelNotifier::Emit(uint64_t targetId, const char* event)
	{
		MS_TRACE();

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;

		this->channel->Send(jsonNotification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event)
	{
		MS_TRACE();

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;

		this->channel->Send(jsonNotification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event, json& data)
	{
		MS_TRACE();

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;
		jsonNotification["data"]     = data;

		this->channel->Send(jsonNotification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event, const std::string& data)
	{
		MS_TRACE();

		std::string notification("{\"targetId\":\"");

		notification.append(targetId);
		notification.append("\",\"event\":\"");
		notification.append(event);
		notification.append("\",\"data\":");
		notification.append(data);
		notification.append("}");

		this->channel->Send(notification);
	}
} // namespace Channel
