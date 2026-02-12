#define MS_CLASS "Channel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelNotifier.hpp"
#include "Logger.hpp"

namespace Channel
{
	/* Instance methods. */

	ChannelNotifier::ChannelNotifier(Channel::ChannelSocket* channel) : channel(channel)
	{
		MS_TRACE();
	}

	void ChannelNotifier::Emit(const std::string& targetId, FBS::Notification::Event event)
	{
		MS_TRACE();

		auto& builder = this->bufferBuilder;
		auto notification = FBS::Notification::CreateNotificationDirect(builder, targetId.c_str(), event);
		auto message =
		  FBS::Message::CreateMessage(builder, FBS::Message::Body::Notification, notification.Union());

		builder.FinishSizePrefixed(message);
		this->channel->Send(builder.GetBufferPointer(), builder.GetSize());
		builder.Clear();
	}
} // namespace Channel
