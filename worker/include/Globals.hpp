#ifndef MS_GLOBALS_HPP
#define MS_GLOBALS_HPP

#include "ChannelMessageRegistrator.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "PayloadChannel/PayloadChannelNotifier.hpp"

class Globals
{
public:
	explicit Globals(
	  ChannelMessageRegistrator* channelMessageRegistrator,
	  Channel::ChannelNotifier* channelNotifier,
	  PayloadChannel::PayloadChannelNotifier* payloadChannelNotifier);
	~Globals();

public:
	ChannelMessageRegistrator* channelMessageRegistrator{ nullptr };
	Channel::ChannelNotifier* channelNotifier{ nullptr };
	PayloadChannel::PayloadChannelNotifier* payloadChannelNotifier{ nullptr };
};

#endif
