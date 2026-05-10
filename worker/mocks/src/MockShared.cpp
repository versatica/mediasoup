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
	}

	TimerHandleInterface* MockShared::CreateTimer(TimerHandleInterface::Listener* /*listener*/)
	{
		return new MockTimerHandle();
	}

	BackoffTimerHandleInterface* MockShared::CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options)
	{
		if (options.label.empty())
		{
			MS_THROW_TYPE_ERROR("options.label must be given");
		}

		const auto& label = options.label;

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
