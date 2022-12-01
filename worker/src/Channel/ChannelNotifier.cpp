#define MS_CLASS "Channel::Notifier"
// #define MS_LOG_DEV_LEVEL 3

#include "Channel/ChannelNotifier.hpp"
#include "FBS/notification_generated.h"
#include "Logger.hpp"

namespace Channel
{
	/* Class variables. */
	flatbuffers::FlatBufferBuilder ChannelNotifier::bufferBuilder;

	ChannelNotifier::ChannelNotifier(Channel::ChannelSocket* channel) : channel(channel)
	{
		MS_TRACE();
	}
} // namespace Channel
