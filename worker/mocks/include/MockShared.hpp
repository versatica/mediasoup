#ifndef MS_MOCKS_MOCK_SHARED_HPP
#define MS_MOCKS_MOCK_SHARED_HPP

// TODO: We need ChannelMessageRegistratorInterface and ChannelNotifierInterface
// classes.
#include "ChannelMessageRegistrator.hpp"
#include "SharedInterface.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "Channel/ChannelSocket.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include "handles/TimerHandleInterface.hpp"

namespace mocks
{
	class MockShared : public SharedInterface
	{
	public:
		explicit MockShared();

		~MockShared() override;

	public:
		ChannelMessageRegistrator* GetChannelMessageRegistrator() const override
		{
			return this->channelMessageRegistrator;
		}

		Channel::ChannelNotifier* GetChannelNotifier() const override
		{
			return this->channelNotifier;
		}

		TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener) const override;

		BackoffTimerHandleInterface* CreateBackoffTimer(
		  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) const override;

	private:
		Channel::ChannelSocket* channelSocket;
		ChannelMessageRegistrator* channelMessageRegistrator;
		Channel::ChannelNotifier* channelNotifier;
	};
} // namespace mocks

#endif
