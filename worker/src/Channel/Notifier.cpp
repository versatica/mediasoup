#define MS_CLASS "Channel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/Notifier.hpp"
#include "Logger.hpp"

namespace Channel
{
	/* Class variables. */

	Channel::UnixStreamSocket* Notifier::channel{ nullptr };

	/* Static methods. */

	void Notifier::ClassInit(Channel::UnixStreamSocket* channel)
	{
		MS_TRACE();

		Notifier::channel = channel;
	}

	void Notifier::Emit(const std::string& targetId, const char* event)
	{
		MS_TRACE();

		MS_ASSERT(Notifier::channel, "channel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;

		Notifier::channel->Send(jsonNotification);
	}

	void Notifier::Emit(const std::string& targetId, const char* event, json& data)
	{
		MS_TRACE();

		MS_ASSERT(Notifier::channel, "channel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;
		jsonNotification["data"]     = data;

		Notifier::channel->Send(jsonNotification);
	}
} // namespace Channel
