#define MS_CLASS "PayloadChannel::PayloadChannelNotification"
// #define MS_LOG_DEV_LEVEL 3

#include "PayloadChannel/PayloadChannelNotification.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace PayloadChannel
{
	/* Class variables. */

	// clang-format off
	absl::flat_hash_map<std::string, PayloadChannelNotification::EventId> PayloadChannelNotification::string2EventId =
	{
		{ "transport.sendRtcp", PayloadChannelNotification::EventId::TRANSPORT_SEND_RTCP },
		{ "producer.send",      PayloadChannelNotification::EventId::PRODUCER_SEND       },
		{ "dataProducer.send",  PayloadChannelNotification::EventId::DATA_PRODUCER_SEND  }
	};
	// clang-format on

	/* Class methods. */

	bool PayloadChannelNotification::IsNotification(const char* msg, size_t msgLen)
	{
		MS_TRACE();

		return (msgLen > 2 && msg[0] == 'n' && msg[1] == ':');
	}

	/* Instance methods. */

	PayloadChannelNotification::PayloadChannelNotification(const char* msg, size_t msgLen)
	{
		MS_TRACE();

		auto info = Utils::String::Split(std::string(msg, msgLen), ':');

		if (info.size() < 1)
			MS_THROW_ERROR("too few arguments");

		this->event = info[0];

		auto eventIdIt = PayloadChannelNotification::string2EventId.find(this->event);

		if (eventIdIt == PayloadChannelNotification::string2EventId.end())
			MS_THROW_ERROR("unknown event '%s'", this->event.c_str());

		this->eventId = eventIdIt->second;

		if (info.size() > 1)
		{
			auto& handlerId = info[1];

			if (handlerId != "undefined")
				this->handlerId = handlerId;
		}

		if (info.size() > 2)
		{
			auto& data = info[2];

			if (data != "undefined")
				this->data = data;
		}
	}

	PayloadChannelNotification::~PayloadChannelNotification()
	{
		MS_TRACE();
	}

	void PayloadChannelNotification::SetPayload(const uint8_t* payload, size_t payloadLen)
	{
		MS_TRACE();

		this->payload    = payload;
		this->payloadLen = payloadLen;
	}
} // namespace PayloadChannel
