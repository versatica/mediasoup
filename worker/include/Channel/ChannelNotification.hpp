#ifndef MS_CHANNEL_NOTIFICATION_HPP
#define MS_CHANNEL_NOTIFICATION_HPP

#include "FBS/notification.h"
#include <string>

namespace Channel
{
	class ChannelNotification
	{
	public:
		using Event = FBS::Notification::Event;

	public:
		explicit ChannelNotification(const FBS::Notification::Notification* notification);
		~ChannelNotification() = default;

	public:
		// Passed by argument.
		Event event;
		// Others.
		const char* eventCStr;
		std::string handlerId;
		const FBS::Notification::Notification* data{ nullptr };
	};
} // namespace Channel

#endif
