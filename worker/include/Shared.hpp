#ifndef MS_SHARED_HPP
#define MS_SHARED_HPP

#include "SharedInterface.hpp"
#include "Channel/ChannelMessageRegistrator.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include "handles/TimerHandleInterface.hpp"

class Shared : public SharedInterface
{
public:
	explicit Shared(
	  Channel::ChannelMessageRegistrator* channelMessageRegistrator,
	  Channel::ChannelNotifier* channelNotifier);

	~Shared() override;

public:
	Channel::ChannelMessageRegistratorInterface* GetChannelMessageRegistrator() const override
	{
		return this->channelMessageRegistrator.get();
	}

	Channel::ChannelNotifier* GetChannelNotifier() const override
	{
		return this->channelNotifier.get();
	}

	TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener) const override;

	BackoffTimerHandleInterface* CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) const override;

private:
	std::unique_ptr<Channel::ChannelMessageRegistrator> channelMessageRegistrator;
	std::unique_ptr<Channel::ChannelNotifier> channelNotifier;
};

#endif
