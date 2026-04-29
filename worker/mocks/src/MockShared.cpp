#include "mocks/include/MockShared.hpp"
// TODO: We need MockChannelNotifier class.
#include "Channel/ChannelSocket.hpp"
// TODO: We need MockBackoffTimerHandle and MockChannelNotifier classes.
#include "handles/BackoffTimerHandle.hpp"
#include "handles/TimerHandle.hpp"

MockShared::MockShared() : channelMessageRegistrator(), channelNotifier()
{
	auto* channelSocket = new Channel::ChannelSocket();

	this->channelMessageRegistrator = new ChannelMessageRegistrator();
	this->channelNotifier           = new Channel::ChannelNotifier(channelSocket);
}

MockShared::~MockShared()
{
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
