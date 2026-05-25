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

	TimerHandleInterface* CreateTimer(TimerHandleInterface::Listener* listener) override;

	BackoffTimerHandleInterface* CreateBackoffTimer(
	  const BackoffTimerHandleInterface::BackoffTimerHandleOptions& options) override;

	uint64_t GetTimeMs() override
	{
		return DepLibUV::GetTimeMs();
	}

	uint64_t GetTimeUs() override
	{
		return DepLibUV::GetTimeUs();
	}

	uint64_t GetTimeNs() override
	{
		return DepLibUV::GetTimeNs();
	}

	int64_t GetTimeMsInt64() override
	{
		return DepLibUV::GetTimeMsInt64();
	}

	int64_t GetTimeUsInt64() override
	{
		return DepLibUV::GetTimeUsInt64();
	}

private:
	std::unique_ptr<Channel::ChannelMessageRegistrator> channelMessageRegistrator;
	std::unique_ptr<Channel::ChannelNotifier> channelNotifier;
};

#endif
