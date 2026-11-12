#define MS_CLASS "Shared"
// #define MS_LOG_DEV_LEVEL 3

#include "Shared.hpp"
#include "handles/BackoffTimerHandle.hpp"
#include "handles/TimerHandle.hpp"
#include "Logger.hpp"

Shared::Shared(
  std::unique_ptr<Channel::ChannelMessageRegistrator> channelMessageRegistrator,
  std::unique_ptr<Channel::ChannelNotifier> channelNotifier)
  : channelMessageRegistrator(std::move(channelMessageRegistrator)),
    channelNotifier(std::move(channelNotifier))
{
	MS_TRACE();
}

Shared::~Shared()
{
	MS_TRACE();
}

TimerHandleInterface* Shared::CreateTimer(TimerHandleInterface::Listener* listener, std::string label)
{
	MS_TRACE();

	return new TimerHandle(listener, std::move(label));
}

BackoffTimerHandleInterface* Shared::CreateBackoffTimer(
  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options)
{
	MS_TRACE();

	return new BackoffTimerHandle(options);
}
