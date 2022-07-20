#define MS_CLASS "PayloadChannel::Notification"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/Notification.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

	/*
	 * msg notification starts with "n:"
	 */
	bool Notification::IsNotification(const char* msg, size_t msgLen)
	{
		MS_TRACE();

		return (msgLen > 2 && msg[0] == 'n' && msg[1] == ':');
	}

	/* Instance methods. */

	Notification::Notification(const char* msg, size_t msgLen)
	{
		MS_TRACE();

		auto info = Utils::String::Split(std::string(msg, msgLen), ':');

		if (info.size() < 1)
			MS_THROW_ERROR("too few arguments");

		this->event = info[0];

		auto eventIdIt = Notification::string2EventId.find(this->event);

		if (eventIdIt == Notification::string2EventId.end())
			MS_THROW_ERROR("unknown event '%s'", this->event.c_str());

		this->eventId = eventIdIt->second;

		if (info.size() > 1)
		{
			auto internal = info[1];

			if (internal != "undefined")
				this->internal = Utils::String::Split(internal, ',');
		}

		if (info.size() > 2)
		{
			auto data = info[2];

			if (data != "undefined")
				this->data = data;
		}
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

	const std::string& Notification::GetNextInternalRoutingId()
	{
		MS_TRACE();

		if (this->internal.size() < this->nextRoutingLevel + 1)
		{
			MS_THROW_ERROR("routing id not found for level %" PRIu8, this->nextRoutingLevel);
		}

		return this->internal[this->nextRoutingLevel++];
	}
} // namespace PayloadChannel
