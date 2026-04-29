#include "mocks/include/MockShared.hpp"
// TODO: We need MockBackoffTimerHandle class.
#include "handles/BackoffTimerHandle.hpp"
// TODO: We need MockTimerHandle class.
#include "handles/TimerHandle.hpp"

namespace mocks
{
	MockShared::MockShared()
	  : channelSocket(new ::Channel::ChannelSocket()),
	    channelMessageRegistrator(new mocks::Channel::MockChannelMessageRegistrator()),
	    channelNotifier(new ::Channel::ChannelNotifier(this->channelSocket.get()))
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
