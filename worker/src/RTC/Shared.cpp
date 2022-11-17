#define MS_CLASS "Shared"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Shared.hpp"
#include "Logger.hpp"

namespace RTC
{
	Shared::Shared(
	  ChannelMessageRegistrator* channelMessageRegistrator,
	  Channel::ChannelNotifier* channelNotifier,
	  PayloadChannel::PayloadChannelNotifier* payloadChannelNotifier)
	  : channelMessageRegistrator(channelMessageRegistrator), channelNotifier(channelNotifier),
	    payloadChannelNotifier(payloadChannelNotifier)
	{
		MS_TRACE();
	}

	Shared::~Shared()
	{
		MS_TRACE();

		delete this->channelMessageRegistrator;
		delete this->channelNotifier;
		delete this->payloadChannelNotifier;
	}
} // namespace RTC
