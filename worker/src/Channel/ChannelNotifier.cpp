#define MS_CLASS "Channel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelNotifier.hpp"
#include "Logger.hpp"

namespace Channel
{
	/* Class variables. */

	thread_local Channel::ChannelSocket* ChannelNotifier::channel{ nullptr };

	/* Static methods. */

	void ChannelNotifier::ClassInit(Channel::ChannelSocket* channel)
	{
		MS_TRACE();

		ChannelNotifier::channel = channel;
	}

	void ChannelNotifier::Emit(uint64_t targetId, const char* event)
	{
		MS_TRACE();

		MS_ASSERT(ChannelNotifier::channel, "channel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;

		ChannelNotifier::channel->Send(jsonNotification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event)
	{
		MS_TRACE();

		MS_ASSERT(ChannelNotifier::channel, "channel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;

		ChannelNotifier::channel->Send(jsonNotification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event, json& data)
	{
		MS_TRACE();

		MS_ASSERT(ChannelNotifier::channel, "channel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;
		jsonNotification["data"]     = data;

		ChannelNotifier::channel->Send(jsonNotification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event, const std::string& data)
	{
		MS_TRACE();

		MS_ASSERT(ChannelNotifier::channel, "channel unset");

		std::string notification("{\"targetId\":\"");

		notification.append(targetId);
		notification.append("\",\"event\":\"");
		notification.append(event);
		notification.append("\",\"data\":");
		notification.append(data);
		notification.append("}");

		ChannelNotifier::channel->Send(notification);
	}
} // namespace Channel
