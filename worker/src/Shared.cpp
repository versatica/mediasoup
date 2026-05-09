#define MS_CLASS "Shared"
// #define MS_LOG_DEV_LEVEL 3

#include "Shared.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "handles/BackoffTimerHandle.hpp"
#include "handles/TimerHandle.hpp"

Shared::Shared(
  Channel::ChannelMessageRegistrator* channelMessageRegistrator,
  Channel::ChannelNotifier* channelNotifier)
  : channelMessageRegistrator(channelMessageRegistrator), channelNotifier(channelNotifier)
{
	MS_TRACE();
}

Shared::~Shared()
{
	MS_TRACE();
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

uint64_t Shared::GetTimeMs() const
{
	MS_TRACE();

	return DepLibUV::GetTimeMs();
}

uint64_t Shared::GetTimeUs() const
{
	MS_TRACE();

	return DepLibUV::GetTimeUs();
}

uint64_t Shared::GetTimeNs() const
{
	MS_TRACE();

	return DepLibUV::GetTimeNs();
}

int64_t Shared::GetTimeMsInt64() const
{
	MS_TRACE();

	return DepLibUV::GetTimeMsInt64();
}

int64_t Shared::GetTimeUsInt64() const
{
	MS_TRACE();

	return DepLibUV::GetTimeUsInt64();
}
