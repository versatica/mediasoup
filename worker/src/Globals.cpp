#define MS_CLASS "Globals"
// #define MS_LOG_DEV_LEVEL 3

#include "Globals.hpp"
#include "Logger.hpp"

Globals::Globals(
  ChannelMessageRegistrator* channelMessageRegistrator,
  Channel::ChannelNotifier* channelNotifier,
  PayloadChannel::PayloadChannelNotifier* payloadChannelNotifier)
  : channelMessageRegistrator(channelMessageRegistrator), channelNotifier(channelNotifier),
    payloadChannelNotifier(payloadChannelNotifier)
{
	MS_TRACE();
}

Globals::~Globals()
{
	MS_TRACE();
}
