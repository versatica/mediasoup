#ifndef MS_SHARED_HPP
#define MS_SHARED_HPP

#include "ChannelMessageRegistrator.hpp"
#include "SharedInterface.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include "handles/TimerHandleInterface.hpp"

class Shared : public SharedInterface
{
public:
	explicit Shared(
	  ChannelMessageRegistrator* channelMessageRegistrator, Channel::ChannelNotifier* channelNotifier);

	~Shared() override;

public:
	ChannelMessageRegistrator* GetChannelMessageRegistrator() const override
	{
		return this->channelMessageRegistrator;
	}

	Channel::ChannelNotifier* GetChannelNotifier() const override
	{
		return this->channelNotifier;
	}

	TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener) const override;

	BackoffTimerHandleInterface* CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) const override;

private:
	ChannelMessageRegistrator* channelMessageRegistrator{ nullptr };
	Channel::ChannelNotifier* channelNotifier{ nullptr };
};

#endif
