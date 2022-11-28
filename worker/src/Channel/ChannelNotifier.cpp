#define MS_CLASS "Channel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelNotifier.hpp"
#include "FBS/notification_generated.h"
#include "Logger.hpp"

namespace Channel
{
	/* Class variables. */
	flatbuffers::FlatBufferBuilder ChannelNotifier::bufferBuilder;

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

		auto& builder = ChannelNotifier::bufferBuilder;
		auto notification =
		  FBS::Notification::CreateJsonNotificationDirect(builder, jsonNotification.dump().c_str());

		Emit(notification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event)
	{
		MS_TRACE();

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;

		auto& builder = ChannelNotifier::bufferBuilder;
		auto notification =
		  FBS::Notification::CreateJsonNotificationDirect(builder, jsonNotification.dump().c_str());

		Emit(notification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event, json& data)
	{
		MS_TRACE();

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;
		jsonNotification["data"]     = data;

		auto& builder = ChannelNotifier::bufferBuilder;
		auto notification =
		  FBS::Notification::CreateJsonNotificationDirect(builder, jsonNotification.dump().c_str());

		Emit(notification);
	}

	void ChannelNotifier::Emit(const std::string& targetId, const char* event, const std::string& data)
	{
		MS_TRACE();

		std::string jsonNotification("{\"targetId\":\"");

		jsonNotification.append(targetId);
		jsonNotification.append("\",\"event\":\"");
		jsonNotification.append(event);
		jsonNotification.append("\",\"data\":");
		jsonNotification.append(data);
		jsonNotification.append("}");

		auto& builder = ChannelNotifier::bufferBuilder;
		auto notification =
		  FBS::Notification::CreateJsonNotificationDirect(builder, jsonNotification.c_str());

		Emit(notification);
	}

	void ChannelNotifier::Emit(flatbuffers::Offset<FBS::Notification::JsonNotification>& notification)
	{
		auto& builder = ChannelNotifier::bufferBuilder;
		auto message  = FBS::Message::CreateMessage(
      builder,
      FBS::Message::Type::JSON_NOTIFICATION,
      FBS::Message::Body::FBS_Notification_JsonNotification,
      notification.Union());

		builder.Finish(message);
		this->channel->Send(builder.GetBufferPointer(), builder.GetSize());
		builder.Reset();
	}
} // namespace Channel
