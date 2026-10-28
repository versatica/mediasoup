#ifndef MS_CHANNEL_NOTIFICATION_HPP
#define MS_CHANNEL_NOTIFICATION_HPP

#include "common.hpp"
#include "FBS/notification.h"
#include <string>

namespace Channel
{
	class ChannelNotification
	{
	public:
		using Event = FBS::Notification::Event;

	public:
		ChannelNotification(const FBS::Notification::Notification* notification, int64_t receivedAtUs);
		~ChannelNotification() = default;

	public:
		// Passed by argument.
		const FBS::Notification::Notification* const data;
		const int64_t receivedAtUs;
		// Others.
		const char* eventCStr;
		std::string handlerId;
	};
} // namespace Channel

#endif
