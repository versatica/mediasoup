#define MS_CLASS "Shared"
// #define MS_LOG_DEV_LEVEL 3

#include "Shared.hpp"
#include "Logger.hpp"
#include "handles/BackoffTimerHandle.hpp"
#include "handles/TimerHandle.hpp"

Shared::Shared(
  ChannelMessageRegistrator* channelMessageRegistrator, Channel::ChannelNotifier* channelNotifier)
  : channelMessageRegistrator(channelMessageRegistrator), channelNotifier(channelNotifier)
{
	MS_TRACE();
}

Shared::~Shared()
{
	MS_TRACE();

	delete this->channelMessageRegistrator;
	delete this->channelNotifier;
}

TimerHandleInterface* Shared::CreateTimer(TimerHandleInterface::Listener* listener) const
{
	MS_TRACE();

	return new TimerHandle(listener);
}

BackoffTimerHandleInterface* Shared::CreateBackoffTimer(
  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) const
{
	MS_TRACE();

	return new BackoffTimerHandle(options);
}
