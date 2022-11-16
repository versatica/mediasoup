#define MS_CLASS "PayloadChannel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/PayloadChannelNotifier.hpp"
#include "Logger.hpp"

namespace PayloadChannel
{
	PayloadChannelNotifier::PayloadChannelNotifier(PayloadChannel::PayloadChannelSocket* payloadChannel)
	  : payloadChannel(payloadChannel)
	{
		MS_TRACE();
	}

	void PayloadChannelNotifier::Emit(
	  const std::string& targetId, const char* event, const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		std::string notification("{\"targetId\":\"");

		notification.append(targetId);
		notification.append("\",\"event\":\"");
		notification.append(event);
		notification.append("\"}");

		this->payloadChannel->Send(notification, payload, payloadLen);
	}

	void PayloadChannelNotifier::Emit(
	  const std::string& targetId, const char* event, json& data, const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;
		jsonNotification["data"]     = data;

		this->payloadChannel->Send(jsonNotification, payload, payloadLen);
	}
} // namespace PayloadChannel
