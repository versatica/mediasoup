#ifndef MS_RTC_SHARED_HPP
#define MS_RTC_SHARED_HPP

#include "ChannelMessageRegistrator.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "PayloadChannel/PayloadChannelNotifier.hpp"

namespace RTC
{
	class Shared
	{
	public:
		explicit Shared(
		  ChannelMessageRegistrator* channelMessageRegistrator,
		  Channel::ChannelNotifier* channelNotifier,
		  PayloadChannel::PayloadChannelNotifier* payloadChannelNotifier);
		~Shared();

	public:
		ChannelMessageRegistrator* channelMessageRegistrator{ nullptr };
		Channel::ChannelNotifier* channelNotifier{ nullptr };
		PayloadChannel::PayloadChannelNotifier* payloadChannelNotifier{ nullptr };
	};
} // namespace RTC

#endif
