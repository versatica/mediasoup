#define MS_CLASS "mocks::MockShared"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/MockShared.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "mocks/include/handles/MockTimerHandle.hpp"

namespace mocks
{
	MockShared::MockShared(std::function<int64_t()> getTimeUsInt64)
	  : getTimeUsInt64(std::move(getTimeUsInt64)),
	    channelSocket(new ::Channel::ChannelSocket()),
	    channelMessageRegistrator(new mocks::Channel::MockChannelMessageRegistrator()),
	    channelNotifier(new ::Channel::ChannelNotifier(this->channelSocket.get()))
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
		if (this->timers.find(label) != this->timers.end())
		{
			MS_THROW_ERROR("a timer with same label already exists [label:%s]", label.c_str());
		}

		auto* timer = new MockTimerHandle(
		  listener,
		  label,
		  // NOTE: The timer mocks take a milliseconds callback, being that the
		  // resolution of the libuv handles they mimic.
		  /*getTimeMs*/
		  [getTimeUsInt64 = this->getTimeUsInt64]()
		  {
			  return static_cast<uint64_t>(getTimeUsInt64() / 1000);
		  },
		  /*onDelete*/
		  [this, label]()
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

		const auto& label = options.label;

		// NOTE: Backoff timers are indexed by label so that tests can retrieve them
		// via GetBackoffTimer(). Two alive timers sharing a label would make that
		// lookup ambiguous.
		if (this->backoffTimers.find(label) != this->backoffTimers.end())
		{
			MS_THROW_ERROR("a backoff timer with same label already exists [label:%s]", label.c_str());
		}

		auto* backoffTimer = new MockBackoffTimerHandle(
		  options,
		  // NOTE: The timer mocks take a milliseconds callback, being that the
		  // resolution of the libuv handles they mimic.
		  /*getTimeMs*/
		  [getTimeUsInt64 = this->getTimeUsInt64]()
		  {
			  return static_cast<uint64_t>(getTimeUsInt64() / 1000);
		  },
		  /*onDelete*/
		  [this, label]()
		  {
			  this->backoffTimers.erase(label);
		  });

		this->backoffTimers[options.label] = backoffTimer;

		return backoffTimer;
	}
} // namespace mocks
