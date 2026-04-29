#include "mocks/include/MockShared.hpp"
// TODO: We need MockChannelNotifier class.
#include "Channel/ChannelSocket.hpp"
// TODO: We need MockBackoffTimerHandle and MockChannelNotifier classes.
#include "handles/BackoffTimerHandle.hpp"
#include "handles/TimerHandle.hpp"

namespace mocks
{
	MockShared::MockShared()
	  : channelSocket(new Channel::ChannelSocket()),
	    channelMessageRegistrator(new Channel::ChannelMessageRegistrator()),
	    channelNotifier(new Channel::ChannelNotifier(this->channelSocket.get()))
	{
	}

	TimerHandleInterface* MockShared::CreateTimer(TimerHandleInterface::Listener* listener) const
	{
		return new TimerHandle(listener);
	}

	BackoffTimerHandleInterface* MockShared::CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) const
	{
		return new BackoffTimerHandle(options);
	}
} // namespace mocks
