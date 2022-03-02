#define MS_CLASS "PayloadChannel::Notification"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/Notification.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace PayloadChannel
{
	/* Class variables. */

	// clang-format off
	absl::flat_hash_map<std::string, Notification::EventId> Notification::string2EventId =
	{
		{ "transport.sendRtcp", Notification::EventId::TRANSPORT_SEND_RTCP },
		{ "producer.send",      Notification::EventId::PRODUCER_SEND       },
		{ "dataProducer.send",  Notification::EventId::DATA_PRODUCER_SEND  }
	};
	// clang-format on

	/* Class methods. */

	bool Notification::IsNotification(json& jsonNotification)
	{
		MS_TRACE();

		auto jsonEventIdIt = jsonNotification.find("event");

		if (jsonEventIdIt == jsonNotification.end() || !jsonEventIdIt->is_string())
			return false;

		auto event = jsonEventIdIt->get<std::string>();

		auto eventIdIt = Notification::string2EventId.find(event);

		if (eventIdIt == Notification::string2EventId.end())
			return false;

		return true;
	}

	/* Instance methods. */

	Notification::Notification(json& jsonNotification)
	{
		MS_TRACE();

		auto jsonEventIdIt = jsonNotification.find("event");

		if (jsonEventIdIt == jsonNotification.end() || !jsonEventIdIt->is_string())
			MS_THROW_ERROR("missing event");

		this->event = jsonEventIdIt->get<std::string>();

		auto eventIdIt = Notification::string2EventId.find(this->event);

		if (eventIdIt == Notification::string2EventId.end())
			MS_THROW_ERROR("unknown event '%s'", this->event.c_str());

		this->eventId = eventIdIt->second;

		auto jsonInternalIt = jsonNotification.find("internal");

		if (jsonInternalIt != jsonNotification.end() && jsonInternalIt->is_object())
			this->internal = *jsonInternalIt;
		else
			this->internal = json::object();

		auto jsonDataIt = jsonNotification.find("data");

		if (jsonDataIt != jsonNotification.end() && jsonDataIt->is_object())
			this->data = *jsonDataIt;
		else
			this->data = json::object();
	}

	Notification::~Notification()
	{
		MS_TRACE();
	}

	void Notification::SetPayload(const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		this->payload    = payload;
		this->payloadLen = payloadLen;
	}
} // namespace PayloadChannel
