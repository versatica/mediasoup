#include "mocks/include/MockShared.hpp"
#include "mocks/include/handles/MockBackoffTimerHandle.hpp"
#include "mocks/include/handles/MockTimerHandle.hpp"

namespace mocks
{
	MockShared::MockShared()
	  : channelSocket(new ::Channel::ChannelSocket()),
	    channelMessageRegistrator(new mocks::Channel::MockChannelMessageRegistrator()),
	    channelNotifier(new ::Channel::ChannelNotifier(this->channelSocket.get()))
	{
	}

	TimerHandleInterface* MockShared::CreateTimer(TimerHandleInterface::Listener* /*listener*/) const
	{
		return new MockTimerHandle();
	}

	BackoffTimerHandleInterface* MockShared::CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) const
	{
		return new MockBackoffTimerHandle(options);
	}
} // namespace mocks
