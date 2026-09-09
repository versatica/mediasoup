#ifndef MS_SHARED_HPP
#define MS_SHARED_HPP

#include "Channel/ChannelMessageRegistrator.hpp"
#include "Channel/ChannelNotifier.hpp"
#include "DepLibUV.hpp"
#include "SharedInterface.hpp"

class Shared : public SharedInterface
{
public:
	explicit Shared(
	  Channel::ChannelMessageRegistrator* channelMessageRegistrator,
	  Channel::ChannelNotifier* channelNotifier);

	~Shared() override;

public:
	Channel::ChannelMessageRegistratorInterface* GetChannelMessageRegistrator() override
	{
		return this->channelMessageRegistrator.get();
	}

	Channel::ChannelNotifier* GetChannelNotifier() override
	{
		return this->channelNotifier.get();
	}

	TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener, std::string label) override;

	BackoffTimerHandleInterface* CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) override;

	int64_t GetTimeMs() override
	{
		return DepLibUV::GetTimeMs();
	}

	int64_t GetTimeUs() override
	{
		return DepLibUV::GetTimeUs();
	}

	int64_t GetNtpOffsetUs() override
	{
		return DepLibUV::GetNtpOffsetUs();
	}

private:
	std::unique_ptr<Channel::ChannelMessageRegistrator> channelMessageRegistrator;
	std::unique_ptr<Channel::ChannelNotifier> channelNotifier;
};

#endif
