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
	absl::flat_hash_map<FBS::Notification::Event, const char*> PayloadChannelNotification::event2String =
	{
		{ FBS::Notification::Event::TRANSPORT_SEND_RTCP, "transport.sendRtcp" },
		{ FBS::Notification::Event::PRODUCER_SEND,       "producer.send" },
		{ FBS::Notification::Event::DATA_PRODUCER_SEND,  "dataProducer.send" },
	};
	// clang-format on

	flatbuffers::FlatBufferBuilder PayloadChannelNotification::bufferBuilder;

	/* Instance methods. */

	PayloadChannelNotification::PayloadChannelNotification(
	  const FBS::Notification::Notification* notification)
	{
		MS_TRACE();

		this->data     = notification;
		this->event    = notification->event();
		this->eventStr = event2String[this->event];

		// Handler ID is optional.
		if (flatbuffers::IsFieldPresent(this->data, FBS::Notification::Notification::VT_HANDLERID))
			this->handlerId = this->data->handlerId()->str();
	}

	PayloadChannelNotification::~PayloadChannelNotification()
	{
		MS_TRACE();
	}
} // namespace PayloadChannel
