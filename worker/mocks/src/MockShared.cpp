#define MS_CLASS "mocks::MockShared"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/MockShared.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace mocks
{
	MockShared::MockShared(std::function<int64_t()> getTimeUs)
	  : getTimeUs(std::move(getTimeUs)),
	    channelSocket(std::make_unique<::Channel::ChannelSocket>()),
	    channelMessageRegistrator(std::make_unique<mocks::Channel::MockChannelMessageRegistrator>()),
	    channelNotifier(std::make_unique<::Channel::ChannelNotifier>(this->channelSocket.get()))
	{
		MS_TRACE();

		// Give the Channel the Shared instance, which it needs to take the arrival
		// time of received notifications.
		this->channelSocket->SetShared(this);
	}

	TimerHandleInterface* MockShared::CreateTimer(
	  TimerHandleInterface::Listener* listener, std::string label)
	{
		MS_TRACE();

		// NOTE: Timers are indexed by label so that tests can retrieve them via
		// GetTimer(). Two alive timers sharing a label would make that lookup
		// ambiguous.
		if (this->timers.contains(label))
		{
			MS_THROW_ERROR("a timer with same label already exists [label:%s]", label.c_str());
		}

		auto* timer = new MockTimerHandle(
		  listener,
		  label,
		  // NOTE: The timer mocks take a milliseconds callback, being that the
		  // resolution of the libuv handles they mimic.
		  /*getTimeMs*/
		  [getTimeUs = this->getTimeUs]() -> int64_t
		  {
			  return getTimeUs() / 1000;
		  },
		  /*onDelete*/
		  [this, label]() -> void
		  {
			  this->timers.erase(label);
		  });

		this->timers[label] = timer;

		return timer;
	}

	BackoffTimerHandleInterface* MockShared::CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options)
	{
		MS_TRACE();

		const std::string& label = options.label;

		// NOTE: Backoff timers are indexed by label so that tests can retrieve them
		// via GetBackoffTimer(). Two alive timers sharing a label would make that
		// lookup ambiguous.
		if (this->backoffTimers.contains(label))
		{
			MS_THROW_ERROR("a backoff timer with same label already exists [label:%s]", label.c_str());
		}

		auto* backoffTimer = new MockBackoffTimerHandle(
		  options,
		  // NOTE: The timer mocks take a milliseconds callback, being that the
		  // resolution of the libuv handles they mimic.
		  /*getTimeMs*/
		  [getTimeUs = this->getTimeUs]() -> int64_t
		  {
			  return getTimeUs() / 1000;
		  },
		  /*onDelete*/
		  [this, label]() -> void
		  {
			  this->backoffTimers.erase(label);
		  });

		this->backoffTimers[label] = backoffTimer;

		return backoffTimer;
	}
} // namespace mocks
