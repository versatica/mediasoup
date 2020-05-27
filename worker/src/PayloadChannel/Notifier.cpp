#define MS_CLASS "PayloadChannel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/Notifier.hpp"
#include "Logger.hpp"

namespace PayloadChannel
{
	/* Class variables. */

	PayloadChannel::UnixStreamSocket* Notifier::payloadChannel{ nullptr };

	/* Static methods. */

	void Notifier::ClassInit(PayloadChannel::UnixStreamSocket* payloadChannel)
	{
		MS_TRACE();

		Notifier::payloadChannel = payloadChannel;
	}

	void Notifier::Emit(
	  const std::string& targetId, const char* event, const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		MS_ASSERT(Notifier::payloadChannel, "payloadChannel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;

		Notifier::payloadChannel->Send(jsonNotification, payload, payloadLen);
	}

	void Notifier::Emit(
	  const std::string& targetId, const char* event, json& data, const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		MS_ASSERT(Notifier::payloadChannel, "payloadChannel unset");

		json jsonNotification = json::object();

		jsonNotification["targetId"] = targetId;
		jsonNotification["event"]    = event;
		jsonNotification["data"]     = data;

		Notifier::payloadChannel->Send(jsonNotification, payload, payloadLen);
	}
} // namespace PayloadChannel
