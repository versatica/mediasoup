#define MS_CLASS "mocks::MockShared"
// #define MS_LOG_DEV_LEVEL 3

#include "mocks/include/MockShared.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "mocks/include/handles/MockTimerHandle.hpp"

namespace mocks
{
	MockShared::MockShared(std::function<uint64_t()> getTimeMs)
	  : getTimeMs(std::move(getTimeMs)),
	    channelSocket(new ::Channel::ChannelSocket()),
	    channelMessageRegistrator(new mocks::Channel::MockChannelMessageRegistrator()),
	    channelNotifier(new ::Channel::ChannelNotifier(this->channelSocket.get()))
	{
		MS_TRACE();
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
		  /*getTimeMs*/ this->getTimeMs,
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
		  /*getTimeMs*/ this->getTimeMs,
		  /*onDelete*/
		  [this, label]()
		  {
			  this->backoffTimers.erase(label);
		  });

		this->backoffTimers[options.label] = backoffTimer;

		return backoffTimer;
	}
} // namespace mocks
