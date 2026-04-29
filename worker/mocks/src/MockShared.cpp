#include "mocks/include/MockShared.hpp"
// TODO: We need MockChannelNotifier class.
#include "Channel/ChannelSocket.hpp"
// TODO: We need MockBackoffTimerHandle and MockChannelNotifier classes.
#include "handles/BackoffTimerHandle.hpp"
#include "handles/TimerHandle.hpp"

namespace mocks
{
	MockShared::MockShared() : channelMessageRegistrator(), channelNotifier()
	{
		this->channelSocket             = new Channel::ChannelSocket();
		this->channelMessageRegistrator = new ChannelMessageRegistrator();
		this->channelNotifier           = new Channel::ChannelNotifier(this->channelSocket);
	}

	MockShared::~MockShared()
	{
		delete this->channelSocket;
		delete this->channelMessageRegistrator;
		delete this->channelNotifier;
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
