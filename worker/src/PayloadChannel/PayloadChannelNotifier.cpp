#define MS_CLASS "PayloadChannel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/PayloadChannelNotifier.hpp"
#include "Logger.hpp"

namespace PayloadChannel
{
	/* Class variables. */

	thread_local PayloadChannel::PayloadChannelSocket* PayloadChannelNotifier::payloadChannel{ nullptr };

	/* Static methods. */

	void PayloadChannelNotifier::ClassInit(PayloadChannel::PayloadChannelSocket* payloadChannel)
	{
		MS_TRACE();

		PayloadChannelNotifier::payloadChannel = payloadChannel;
	}

	void PayloadChannelNotifier::Emit(
	  const std::string& targetId, const char* event, const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		MS_ASSERT(PayloadChannelNotifier::payloadChannel, "payloadChannel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;

		PayloadChannelNotifier::payloadChannel->Send(jsonNotification, payload, payloadLen);
	}

	void PayloadChannelNotifier::Emit(
	  const std::string& targetId, const char* event, json& data, const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		MS_ASSERT(PayloadChannelNotifier::payloadChannel, "payloadChannel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;
		jsonNotification["data"]     = data;

		PayloadChannelNotifier::payloadChannel->Send(jsonNotification, payload, payloadLen);
	}
} // namespace PayloadChannel
