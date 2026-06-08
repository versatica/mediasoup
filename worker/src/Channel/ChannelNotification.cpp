#define MS_CLASS "Channel::ChannelNotification"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelNotification.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <ankerl/unordered_dense.h>

namespace Channel
{
	/* Static. */

	// clang-format off
	static const ankerl::unordered_dense::map<FBS::Notification::Event, const char*> Event2String =
	{
		{ FBS::Notification::Event::WORKER_CLOSE,        "worker.close"       },
		{ FBS::Notification::Event::TRANSPORT_SEND_RTCP, "transport.sendRtcp" },
		{ FBS::Notification::Event::PRODUCER_SEND,       "producer.send"      },
		{ FBS::Notification::Event::DATAPRODUCER_SEND,   "dataProducer.send"  },
	};
	// clang-format on

	/* Instance methods. */

	ChannelNotification::ChannelNotification(const FBS::Notification::Notification* notification)
	{
		MS_TRACE();

		this->data  = notification;
		this->event = notification->event();

		auto eventCStrIt = Event2String.find(this->event);

		if (eventCStrIt == Event2String.end())
		{
			MS_THROW_ERROR("unknown event '%" PRIu8 "'", static_cast<uint8_t>(this->event));
		}

		this->eventCStr = eventCStrIt->second;
		this->handlerId = this->data->handlerId()->str();
	}
} // namespace Channel
